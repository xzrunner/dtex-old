#include "dtex_loader.h"
#include "dtex_desc_loader.h"
#include "dtex_texture_loader.h"
#include "dtex_stream_import.h"
#include "dtex_log.h"
#include "dtex_file.h"
#include "dtex_package.h"
#include "dtex_typedef.h"
#include "dtex_texture.h"
#include "dtex_ej_utility.h"
#include "dtex_resource.h"

#ifdef _LZ_ALLOC
#include <LzAlloc.h>
#else
#include <Alloc.h>
#endif // _LZ_ALLOC
#include <LzmaDec.h>

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define PACKAGE_SIZE 512

struct dtex_loader {
	struct dtex_package packages[PACKAGE_SIZE];	// todo malloc
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

void 
dtexloader_release(struct dtex_loader* loader) {
	for (int i = 0; i < loader->pkg_size; ++i) {
		struct dtex_package* pkg = &loader->packages[i];
		dtex_package_release(pkg);
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
_unpack_file(struct dtex_loader* loader, struct dtex_file* file, void (*unpack_func)(struct dtex_import_stream* is, void* ud), void* ud) {
	int32_t sz = 0;
	dtex_file_read(file, &sz, sizeof(sz));
	if (sz < 0) {
		sz = -sz;
		char* buf = NULL;
		if (loader) {
			_buf_reserve(loader, sz);
			buf = loader->buf;
		} else {
			buf = (char*)malloc(sz);
			if (!buf) {
				dtex_fault("_unpack_file malloc fail.\n");
			}
		}
		if (dtex_file_read(file, buf, sz) != 1) {
			if (!loader) { free(buf); }
			dtex_fault("Invalid uncompress data source\n");
		}

		struct dtex_import_stream is;
		is.stream = buf;
		is.size = sz;
		unpack_func(&is, ud);

		if (!loader) { free(buf); }
	} else {
		uint8_t ori_sz_arr[4];
		dtex_file_read(file, ori_sz_arr, sizeof(ori_sz_arr));
		dtex_file_seek_from_cur(file, -sizeof(ori_sz_arr));
		size_t ori_sz = ori_sz_arr[0] << 24 | ori_sz_arr[1] << 16 | ori_sz_arr[2] << 8 | ori_sz_arr[3];
		size_t need_sz = sz + 7 + ori_sz;

		char* buf = NULL;
		if (loader) {
			_buf_reserve(loader, need_sz);
			buf = loader->buf;
		} else {
			buf = (char*)malloc(need_sz);
			if (!buf) {
				dtex_fault("_unpack_file malloc fail.\n");
			}
		}

		struct block* block = (struct block*)buf;
		if (sz <= 4 + LZMA_PROPS_SIZE || dtex_file_read(file, block, sz) != 1) {
			if (!loader) { free(buf); }
			dtex_fault("Invalid compress data source\n");
		}

		unsigned char* buffer = (unsigned char*)(buf + ((sz + 3) & ~3));
		size_t compressed_sz = sz - sizeof(block->size) - LZMA_PROPS_SIZE;

		int r = _lzma_uncompress(buffer, &ori_sz, block->data, &compressed_sz, block->prop, LZMA_PROPS_SIZE);
		if (r != SZ_OK) {
			dtex_fault("Uncompress error %d\n",r);
		}

		struct dtex_import_stream is;
		is.stream = buffer;
		is.size = ori_sz;
		unpack_func(&is, ud);

		if (!loader) { free(buf); }
	}
}

struct unpack_pkg_params {
	struct dtex_package* pkg;
	int file_format;
	float scale;
};

struct unpack_tex_params {
	struct dtex_package* pkg;
	int load_tex_idx;
	bool create_by_ej;
};

struct unpack_pretex_params {
	struct dtex_package* pkg;
	float scale;
};

struct relocate_quad_texid_params {
	int from;
	int to;
};

static inline void
_relocate_quad_texid(int pic_id, struct ej_pack_picture* ej_pic, void* ud) {
	struct relocate_quad_texid_params* params = (struct relocate_quad_texid_params*)ud;
	for (int i = 0; i < ej_pic->n; ++i) {
		struct pack_quad* ej_q = &ej_pic->rect[i];
		if (ej_q->texid == params->from) {
			ej_q->texid = params->to;
		}
	}
}

static inline void
_unpack_memory_to_pkg(struct dtex_import_stream* is, void* ud) {
	struct unpack_pkg_params* params = (struct unpack_pkg_params*)ud;
	struct dtex_package* pkg = params->pkg;
	switch (params->file_format) {
	case FILE_EPE:
		dtex_load_epe(is, pkg, params->scale);
		break;
	case FILE_RRP:
		pkg->rrp_pkg = dtex_load_rrp(is);
		break;
	case FILE_PTS:
		pkg->pts_pkg = dtex_load_pts(is);
		break;
	case FILE_RRR:
		pkg->rrr_pkg = dtex_load_rrr(is);
		break;
	case FILE_B4R:
		pkg->b4r_pkg = dtex_load_b4r(is);
		break;
	default:
		dtex_fault("_unpack_memory_to_pkg unknown file type %d", params->file_format);
	}
}

static inline void
_unpack_memory_to_texture(struct dtex_import_stream* is, void* ud) {
	struct unpack_tex_params* params = (struct unpack_tex_params*)ud;
	struct dtex_package* pkg = params->pkg;
	assert(params->load_tex_idx < pkg->texture_count);
	struct dtex_texture* tex = pkg->textures[params->load_tex_idx];
	assert(tex);
	dtex_load_texture_all(is, tex, params->create_by_ej);
}

static inline void
_unpack_memory_to_preload_texture(struct dtex_import_stream* is, void* ud) {
	struct unpack_pretex_params* params = (struct unpack_pretex_params*)ud;

	struct dtex_package* pkg = params->pkg;

	struct dtex_texture* tex = dtex_texture_create_raw();
	if (!tex) {
		dtex_fault("_unpack_memory_to_preload_texture dtex_texture_create_raw err.");
	}

	struct relocate_quad_texid_params relocate_params;
	relocate_params.from = pkg->texture_count;
	relocate_params.to = tex->uid;
	dtex_ej_pkg_traverse(pkg->ej_pkg, _relocate_quad_texid, &relocate_params);

	dtex_load_texture_only_desc(is, tex, params->scale);
	pkg->textures[pkg->texture_count++] = tex;
}

static inline struct dtex_package*
_find_package(struct dtex_loader* loader, const char* name) {
	for (int i = 0; i < loader->pkg_size; ++i) {
		struct dtex_package* pkg = &loader->packages[i];
		if (strcmp(name, pkg->name) == 0) {
			return pkg;
		}
	}
	return NULL;
}

static inline struct dtex_package*
_new_package(struct dtex_loader* loader, const char* name, const char* filepath) {
	if (loader->pkg_size >= PACKAGE_SIZE) {
		dtex_fault("_new_package: loader->pack_size >= PACKAGE_SIZE\n");
	}	

	struct dtex_package* pkg = &loader->packages[loader->pkg_size++];
	memset(pkg, 0, sizeof(*pkg));

	pkg->name = (char*)malloc(strlen(name) + 1);
	strcpy(pkg->name, name);
	pkg->name[strlen(pkg->name)] = 0;

	pkg->filepath = (char*)malloc(strlen(filepath) + 1);
	strcpy(pkg->filepath, filepath);
	pkg->filepath[strlen(pkg->filepath)] = 0;

	return pkg;
}

struct dtex_package* 
dtex_load_pkg(struct dtex_loader* loader, const char* name, const char* filepath, int format, float scale, int lod) {
	assert(format != FILE_EPT);
	char path_full[strlen(filepath) + 10];
	dtex_get_resource_filepath(filepath, format, path_full);

	struct dtex_file* file = dtex_file_open(path_full, "rb");
	if (!file) {
		dtex_fault("dtexloader_preload_pkg: can't open file %s\n", path_full);
	}

	struct dtex_package* pkg = _find_package(loader, name);
	if (!pkg) {
		pkg = _new_package(loader, name, filepath);
	}

	pkg->LOD = lod;

	struct unpack_pkg_params params;
	params.pkg = pkg;
	params.file_format = format;
	params.scale = scale;
	_unpack_file(loader, file, &_unpack_memory_to_pkg, &params);

	dtex_file_close(file);

	return pkg;
}

void 
dtex_preload_texture(struct dtex_loader* loader, struct dtex_package* pkg, int idx, float scale) {
	char path_full[strlen(pkg->filepath) + 10];
	dtex_get_texture_filepath(pkg->filepath, idx, pkg->LOD, path_full);

	struct dtex_file* file = dtex_file_open(path_full, "rb");
	if (!file) {
		dtex_fault("dtex_preload_texture: can't open file %s\n", path_full);
	}

	struct unpack_pretex_params params;
	params.pkg = pkg;
	params.scale = scale;
	_unpack_file(loader, file, &_unpack_memory_to_preload_texture, &params);

	dtex_file_close(file);
}

void 
dtex_load_texture(struct dtex_loader* loader, struct dtex_package* pkg, int idx, float scale, bool create_by_ej) {
	char path_full[strlen(pkg->filepath) + 10];
	dtex_get_texture_filepath(pkg->filepath, idx, pkg->LOD, path_full);

	struct dtex_file* file = dtex_file_open(path_full, "rb");
	if (!file) {
		dtex_fault("dtex_preload_texture: can't open file %s\n", path_full);
	}

	struct unpack_tex_params params;
	params.pkg = pkg;
	params.load_tex_idx = idx;
	params.create_by_ej = create_by_ej;
	_unpack_file(loader, file, &_unpack_memory_to_texture, &params);

	dtex_file_close(file);
}

void 
dtex_load_file(const char* filepath, void (*unpack_func)(struct dtex_import_stream* is, void* ud), void* ud) {
	struct dtex_file* file = dtex_file_open(filepath, "rb");
	if (!file) {
		dtex_fault("dtexloader_preload_pkg: can't open file %s\n", filepath);
	}

	_unpack_file(NULL, file, unpack_func, ud);

	dtex_file_close(file);
}

struct dtex_package* 
dtex_fetch_pkg(struct dtex_loader* loader, int idx) {
	if (idx < 0 || idx >= loader->pkg_size) {
		return NULL;
	} else {
		return &loader->packages[idx];
	}
}

struct dtex_package* 
dtex_query_pkg(struct dtex_loader* loader, const char* name) {
	for (int i = 0; i < loader->pkg_size; ++i) {
		if (strcmp(name, loader->packages[i].name) == 0) {
			return &loader->packages[i];
		}
	}
	return NULL;
}

//struct dtex_raw_tex* 
//dtexloader_load_image(const char* filepath) {
//	if (strstr(filepath, ".png")) {
//		int w, h, c, f;
//		uint8_t* p = dtex_png_read(filepath, &w, &h, &c, &f);
//
//		GLuint tex = dtex_prepare_texture(GL_TEXTURE0);
//		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)w, (GLsizei)h, 0, GL_RGBA, GL_UNSIGNED_BYTE, p);
//
//		free(p);
//
//		struct dtex_raw_tex* ret = (struct dtex_raw_tex*)malloc(sizeof(struct dtex_raw_tex));
//		ret->format = TEXTURE8;
//		ret->width = w;
//		ret->height = h;
//		ret->id = tex;
//		ret->id_alpha = 0;
//		return ret;
//	} else {
//		return NULL;
//	}
//}

//struct dtex_rrp* 
//dtexloader_query_rrp(struct dtex_loader* dtex, struct ej_sprite_pack* ej_pkg) {
//    if (!dtex) {
//        return NULL;
//    }
//    
//	static struct dtex_package* cache_pkg = NULL;
//	if (cache_pkg && cache_pkg->ej_pkg == ej_pkg) {
//		return cache_pkg->rrp_pkg;
//	}
//
//	for (int i = 0; i < dtex->pkg_size; ++i) {
//		struct dtex_package* pkg = dtex->packages[i];
//		// todo
//		// if (pkg->ej_pkg == ej_pkg) {
//			cache_pkg = pkg;
//			return pkg->rrp_pkg;
//		// }
//	}
//	return NULL;
//}
//
//struct dtex_pts* 
//dtexloader_query_pts(struct dtex_loader* dtex, struct ej_sprite_pack* ej_pkg) {
//	static struct dtex_package* cache_pkg = NULL;
//	if (cache_pkg && cache_pkg->ej_pkg == ej_pkg) {
//		return cache_pkg->pts_pkg;
//	}
//
//	for (int i = 0; i < dtex->pkg_size; ++i) {
//		struct dtex_package* pkg = dtex->packages[i];
//		// todo
//		// if (pkg->ej_pkg == ej_pkg) {
//			cache_pkg = pkg;
//			return pkg->pts_pkg;
//		// }
//	}
//	return NULL;
//}
