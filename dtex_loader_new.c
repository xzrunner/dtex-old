#include "dtex_loader_new.h"
#include "dtex_desc_loader.h"
#include "dtex_texture_loader.h"
#include "dtex_texture_pool.h"
#include "dtex_log.h"
#include "dtex_file.h"
#include "dtex_rrp.h"
#include "dtex_pts.h"
#include "dtex_rrr.h"
#include "dtex_b4r.h"

#include "LzmaDec.h"
#ifdef _MSC_VER
#include "Alloc.h"
#else
#include "LzAlloc.h"
#endif // _MSC_VER

#include <stdlib.h>
#include <string.h>

#define PACKAGE_SIZE 512

struct dtex_loader {
	struct dtex_package* packages[PACKAGE_SIZE];	// todo malloc
	int pkg_size;

	// cache memory buf
	uint8_t* buf;
	size_t buf_size;
	// cache tex id
	// todo
};

struct dtex_loader* 
dtexloader_create() {
	size_t sz = sizeof(struct dtex_loader);
	struct dtex_loader* loader = (struct dtex_loader*)malloc(sz);
	memset(loader, 0, sz);
	return loader;
}

static inline void
_release_package(struct dtex_package* pkg) {
	free(pkg->ej_pkg);
	dtex_rrp_release(pkg->rrp_pkg);
	dtex_pts_release(pkg->pts_pkg);
	dtex_rrr_release(pkg->rrr_pkg);
	dtex_b4r_release(pkg->b4r_pkg);

	free(pkg->name);

	memset(pkg, 0, sizeof(*pkg));
}

void 
dtexloader_release(struct dtex_loader* loader) {
	for (int i = 0; i < loader->pkg_size; ++i) {
		struct dtex_package* pkg = loader->packages[i];
		_release_package(pkg);
	}

	free(loader->buf); loader->buf = NULL;
	loader->buf_size = 0;

	free(loader);
}

#define LZMA_PROPS_SIZE 5

static void *SzAlloc(void *p, size_t size) { p = p; return MyAlloc(size); }
static void SzFree(void *p, void *address) { p = p; MyFree(address); }
static ISzAlloc g_Alloc = { SzAlloc, SzFree };

static inline int
_lzma_uncompress(unsigned char *dest, size_t  *destLen, const unsigned char *src, size_t  *srcLen, 
	const unsigned char *props, size_t propsSize)
{
	ELzmaStatus status;
	return LzmaDecode(dest, destLen, src, srcLen, props, (unsigned)propsSize, LZMA_FINISH_ANY, &status, &g_Alloc);
}

struct block {
	uint8_t size[4];
	uint8_t prop[LZMA_PROPS_SIZE];
	uint8_t data[119];
};

static inline void
_buf_reserve(struct dtex_loader* loader, uint32_t sz) {
	if (sz <= loader->buf_size) {
		return;
	}

	int new_sz = sz * 1.5f;
	unsigned char* buf = malloc(new_sz);
	if (buf == NULL) {
		return;
	}
	loader->buf_size = new_sz;
	free(loader->buf);
	loader->buf = buf;

	dtex_info("dtex_loader buf size:%0.1fM\n", (float)new_sz / 1024 / 1024);
}

static inline void
_unpack_file(struct dtex_loader* dtex, struct dtex_file* file, void (*unpack_func)(), void* ud) {
	int32_t sz = 0;
	dtex_file_read(file, &sz, sizeof(sz));
	if (sz < 0) {
		sz = -sz;
		_buf_reserve(dtex, sz);
		if (dtex_file_read(file, dtex->buf, sz) != 1) {
			dtex_fault("Invalid uncompress data source\n");
		}
		unpack_func(dtex->buf, sz, ud);
	} else {
		uint8_t ori_sz_arr[4];
		dtex_file_read(file, ori_sz_arr, sizeof(ori_sz_arr));
		dtex_file_seek_from_cur(file, -sizeof(ori_sz_arr));
		size_t ori_sz = ori_sz_arr[0] << 24 | ori_sz_arr[1] << 16 | ori_sz_arr[2] << 8 | ori_sz_arr[3];
		size_t need_sz = sz + 7 + ori_sz;
		_buf_reserve(dtex, need_sz);

		struct block* block = (struct block*)dtex->buf;
		if (sz <= 4 + LZMA_PROPS_SIZE || dtex_file_read(file, block, sz) != 1) {
			dtex_fault("Invalid compress data source\n");
		}

		unsigned char* buffer = (unsigned char*)(dtex->buf + ((sz + 3) & ~3));
		size_t compressed_sz = sz - sizeof(block->size) - LZMA_PROPS_SIZE;

		int r = _lzma_uncompress(buffer, &ori_sz, block->data, &compressed_sz, block->prop, LZMA_PROPS_SIZE);
		if (r != SZ_OK) {
			dtex_fault("Uncompress error %d\n",r);
		}
		unpack_func(buffer, ori_sz, ud);		
	}
}

struct unpack2pkg_params {
	struct dtex_package* pkg;
	int type;
	int load_tex_idx;
};

static inline void
_unpack_memory_to_pkg(int block_idx, uint8_t* buf, size_t sz, void* ud) {
	struct unpack2pkg_params* params = (struct unpack2pkg_params*)ud;

	if (params->type == TEXTURE4 || params->type == TEXTURE8 ||
		params->type == PVRTC || params->type == PKMC) {
		struct dtex_raw_tex tex;
		switch (params->type) {
			case TEXTURE4: case TEXTURE8:
				dtex_load_png(buf, params->type, &tex);
				break;
			case PVRTC:
				dtex_load_pvr(buf, sz, &tex);
				break;
			case PKMC:
				dtex_load_etc1(buf, &tex);
				break;
		}
		dtex_pool_add(&tex);
	} else {
		struct dtex_package* pkg = params->pkg;
		switch (params->type) {
		case DETAIL:
			pkg->ej_pkg = dtex_load_epe(buf);
			break;
		case RRP:
			pkg->rrp_pkg = dtex_load_rrp(buf, sz);
			break;
		case PTS:
			pkg->pts_pkg = dtex_load_pts(buf, sz);
			break;
		case RRR:
			pkg->rrr_pkg = dtex_load_rrr(buf, sz);
			break;
		case B4R:
			pkg->b4r_pkg = dtex_load_b4r(buf, sz);
			break;
		}
	}
}

struct dtex_package* 
dtexloader_preload_pkg(struct dtex_loader* loader, const char* name, const char* path, int type) {
	
}

struct dtex_raw_tex* 
dtexloader_load_ept(struct dtex_loader* loader, struct dtex_package* pkg, int idx) {
	
}
