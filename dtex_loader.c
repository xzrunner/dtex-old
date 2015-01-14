#include "dtex_loader.h"
#include "dtex_rrp.h"
#include "dtex_pts.h"

#include "package.h"
#include "fault.h"
#include "platform.h"

#include <LzmaDec.h>
#include <7zAlloc.h>

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

// todo define max texture in one package
#define MAX_TEX_PER_PKG 4

#define PACKAGE_SIZE 512

#define TASK_SIZE 16

#define LZMA_PROPS_SIZE 5

#define IS_POT(x) (((x) & ((x) -1)) == 0)

struct load_task {
	bool is_epp;

	struct ej_package* pkg;
	struct dtex_rect* rect;
	int spr_id;
	int tex_idx;
	struct dtex_raw_tex texture;

	uint8_t* buf;
	size_t sz;
};

struct dtex_loader {
	struct dtex_package* packages[PACKAGE_SIZE];
	int pkg_size;

	// cache memory buf
	uint8_t* buf;
	size_t buf_size;
	// cache tex id
	// todo

	// todo cache task's buf
	struct load_task* tasks[TASK_SIZE];
	int task_size;
};

struct dtex_loader* 
dtexloader_create() {
	size_t sz = sizeof(struct dtex_loader);
	struct dtex_loader* loader = (struct dtex_loader*)malloc(sz);
	memset(loader, 0, sz);
	// todo not malloc again
	for (int i = 0; i < TASK_SIZE; ++i) {
		struct load_task* task = malloc(sizeof(*task));
		memset(task, 0, sizeof(*task));
		loader->tasks[i] = task;
	}
	return loader;
}

static inline void
_release_texture(struct dtex_raw_tex* tex) {
	if (tex->id != 0) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDeleteTextures(1, &tex->id);
		tex->id = 0;
	}
    if (tex->id_alpha != 0) {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDeleteTextures(1, &tex->id_alpha);
		tex->id_alpha = 0;
    }
}

static inline void
_release_package(struct dtex_package* pkg) {
	free(pkg->ej_pkg); pkg->ej_pkg = NULL;
	free(pkg->rrp_pkg); pkg->rrp_pkg = NULL;
	free(pkg->pts_pkg); pkg->pts_pkg = NULL;

	free(pkg->name); pkg->name = NULL;
	free(pkg->filepath); pkg->filepath = NULL;

	for (int i = 0; i < pkg->tex_size; ++i) {
		_release_texture(&pkg->textures[i]);
	}
	pkg->tex_size = 0;

	free(pkg);
}

void 
dtexloader_release(struct dtex_loader* dtex) {
	for (int i = 0; i < dtex->pkg_size; ++i) {
		struct dtex_package* pkg = dtex->packages[i];
		_release_package(pkg);
	}

	free(dtex->buf); dtex->buf = NULL;
	dtex->buf_size = 0;

	free(dtex);
}

static inline void
_load_epp(uint8_t* buffer, size_t sz, struct dtex_package* pkg, int format) {
	if (pkg->tex_size >= DTEX_PACK_TEX_SZ) {
		fault("pkg->tex_size >= DTEX_PACK_TEX_SZ\n");
	}

	struct dtex_raw_tex* tex = &pkg->textures[pkg->tex_size++];
	tex->id = tex->id_alpha = 0;
	if (format == PVRTC) {
		tex->width = buffer[1] | buffer[2] << 8;
		tex->height = buffer[3] | buffer[4] << 8;
	} else {
		tex->width = buffer[0] | buffer[1] << 8;
		tex->height = buffer[2] | buffer[3] << 8;		
	}
	tex->format = format;
}

static inline int
_comp_export(const void* a, const void* b) {
	const struct export* aa = a;
	const struct export* bb = b;
	return strcmp(aa->name,bb->name);
}

static inline void
_sort_ep(struct ejoypic* ep) {
	qsort(ep->export, ep->export_n, sizeof(struct export), _comp_export);
}

static inline void
_load_epd(uint8_t* buffer, size_t sz, struct dtex_package* pkg) {
	assert(pkg->ej_pkg == NULL);

	struct ej_package* ej_pkg = malloc(sizeof(*pkg) + sizeof(struct texture) * MAX_TEX_PER_PKG);
	ej_pkg->name = NULL;
	ej_pkg->texture_n = 0;
	ej_pkg->ep = NULL;

	uint32_t cap = buffer[0] | buffer[1]<<8 | buffer[2]<<16 | buffer[3]<<24;
	struct ejoypic* ep = ejoypic_import(buffer + 4, sz - 4, cap);
	if (ep == NULL) {
		fault("Error in epd");
		return; // for static analyze
	}
	_sort_ep(ep);
	ej_pkg->ep = ep;

	pkg->ej_pkg = ej_pkg;
}

static inline void
_load_rrp(uint8_t* buffer, size_t sz, struct dtex_package* pkg) {
	assert(pkg->rrp_pkg == NULL);

	uint32_t cap = buffer[0] | buffer[1]<<8 | buffer[2]<<16 | buffer[3]<<24;
	struct dtex_rrp* rrp = dtex_rrp_create(buffer + 4, sz - 4, cap);
	if (rrp == NULL) {
		fault("Error create rrp.\n");
	}
	pkg->rrp_pkg = rrp;
}

static inline void
_load_pts(uint8_t* buffer, size_t sz, struct dtex_package* pkg) {
	assert(pkg->pts_pkg == NULL);

	uint32_t cap = buffer[0] | buffer[1]<<8 | buffer[2]<<16 | buffer[3]<<24;
	struct dtex_pts* pts = dtex_pts_create(buffer + 4, sz - 4, cap);
	if (pts == NULL) {
		fault("Error create rrp.\n");
	}
	pkg->pts_pkg = pts;
}

static inline GLuint
_gen_texture_id(int tex_no) {
	GLuint texid = 0;

	glActiveTexture(tex_no);
	glGenTextures(1, &texid);
    
	glBindTexture(GL_TEXTURE_2D, texid);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	return texid;
}

static inline GLuint
_texture_create(uint8_t* data, int format, int width, int height) {
	if ((format == TEXTURE8) || (IS_POT(width) && IS_POT(height))) {
		glPixelStorei(GL_UNPACK_ALIGNMENT,4);
	} else {
		glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	}

	GLuint texid = _gen_texture_id(GL_TEXTURE0);
	switch(format) {
	case TEXTURE8:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		break;
	case TEXTURE4:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height, 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, data);
		break;
	default:
		return 0;
	}
	return texid;
}

static inline GLuint
_pvr_texture_create(uint8_t* data, size_t sz, int internal_format, int width, int height) {
	GLuint texid = _gen_texture_id(GL_TEXTURE0);
	uint8_t* ptr = data;
	for (int i = 0; ptr - data < sz; ++i) {
		int ori_sz = ptr[0] | ptr[1] << 8 | ptr[2] << 16 | ptr[3] << 24;
    	glCompressedTexImage2D(GL_TEXTURE_2D, i, internal_format, width, height, 0, ori_sz, ptr+4);
    	ptr += 4 + ori_sz;
	}
	return texid;
}

static inline void
_pkm_texture_create(uint8_t* data, int width, int height, GLuint* id_rgb, GLuint* id_alpha) {
#ifdef __ANDROID__	
	size_t sz = (width * height) >> 1;

	*id_rgb = _gen_texture_id(GL_TEXTURE0);
  	glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_ETC1_RGB8_OES, width, height, 0, sz, data);	

  	*id_alpha = _gen_texture_id(GL_TEXTURE1);
  	glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_ETC1_RGB8_OES, width, height, 0, sz, data+sz);  	
#endif
}

static inline void
_do_load_task(struct load_task* task) {
	// only support epp now
	if (!task->is_epp) {
		return;
	}

	size_t sz = task->sz;
	uint8_t* buf = task->buf;
	int format = buf[0];

	int width, height;
	if (format == PVRTC) {
		width = buf[2] | buf[3] << 8;
		height = buf[4] | buf[5] << 8;
	} else {
		width = buf[1] | buf[2] << 8;
		height = buf[3] | buf[4] << 8;
	}

	struct dtex_raw_tex* tex = &task->texture;
	tex->id = tex->id_alpha = 0;
	tex->width = width;
	tex->height = height;
	tex->format = format;
	switch (format) {
	case TEXTURE4: case TEXTURE8:
		tex->id = _texture_create(buf+5, format, width, height);
		break;
	case PVRTC:
		tex->id = _pvr_texture_create(buf+6, sz-6, buf[1], width, height);
		break;
    case PKMC:
    	_pkm_texture_create(buf+5, width, height, &tex->id, &tex->id_alpha);
		break;
	default:
		fault("Invalid package format %d\n",format);		
	}
}

static inline void
_load_png(uint8_t* buffer, int format, struct dtex_raw_tex* tex) {
	assert(tex->width == (buffer[0] | buffer[1] << 8)
		&& tex->height == (buffer[2] | buffer[3] << 8));
	tex->id = _texture_create(buffer+4, format, tex->width, tex->height);
}

static inline void
_load_pvr(uint8_t* buffer, size_t sz, struct dtex_raw_tex* tex) {
	assert(tex->width == (buffer[1] | buffer[2] << 8)
		&& tex->height == (buffer[3] | buffer[4] << 8));
	int format = buffer[0];
	int internal_format = 0;
#ifdef __APPLE__ 
	if (format == 4) {
        internal_format = GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
	} else if (format == 2) {
		internal_format = GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
	} else {
		assert(0);
	}
#endif
	tex->id = _pvr_texture_create(buffer+5, sz-5, internal_format, tex->width, tex->height);
}

static inline void
_load_pkm(uint8_t* buffer, struct dtex_raw_tex* tex) {
	assert(tex->width == (buffer[0] | buffer[1] << 8)
		&& tex->height == (buffer[2] | buffer[3] << 8));
	_pkm_texture_create(buffer+4, tex->width, tex->height, &tex->id, &tex->id_alpha);
}

struct unpack2pkg_params {
	struct dtex_package* pkg;
	int load_tex_idx;
};

static inline void
_unpack_memory_to_pkg(uint8_t* buffer, size_t sz, void* ud) {
	struct unpack2pkg_params* params = (struct unpack2pkg_params*)ud;

	int format = buffer[0];
	if (params->load_tex_idx >= 0) {
		struct dtex_raw_tex* tex = &params->pkg->textures[params->load_tex_idx];
		assert(tex->format == format);
		switch (format) {
		case TEXTURE4: case TEXTURE8:
			_load_png(buffer+1, format, tex);
			break;
		case PVRTC:
			_load_pvr(buffer+1, sz-1, tex);
			break;
		case PKMC:
			_load_pkm(buffer+1, tex);
			break;
		default:
			fault("Invalid package format %d\n",format);
		}
		++params->load_tex_idx;
	} else {
		switch (format) {
		case TEXTURE4: case TEXTURE8: case PVRTC: case PKMC:
			_load_epp(buffer+1, sz-1, params->pkg, format);
			break;
		case DETAIL:
			_load_epd(buffer+1, sz-1, params->pkg);
			break;
		case RRP:
			_load_rrp(buffer+1, sz-1, params->pkg);
			break;
		case PTS:
			_load_pts(buffer+1, sz-1, params->pkg);
			break;
		default:
			fault("Invalid package format %d\n",format);
		}
	}
}

struct unpack2task_params {
	struct dtex_loader* dtex;

	bool is_epp;

	struct ej_package* pkg;	
	struct dtex_rect* rect;
	int spr_id;
	int tex_idx;
};

static inline void
_unpack_memory_to_task(uint8_t* buffer, size_t sz, void* ud) {
	struct unpack2task_params* params = (struct unpack2task_params*)ud;

	assert(params->dtex->task_size < TASK_SIZE);
	struct load_task* task = params->dtex->tasks[params->dtex->task_size++];

	task->is_epp = params->is_epp;
	task->pkg = params->pkg;
	task->rect = params->rect;
	task->spr_id = params->spr_id;
	task->tex_idx = params->tex_idx;

	if (task->sz == 0 || sz > task->sz) {
//        printf("alloc buf\n");
		free(task->buf);
		task->buf = (uint8_t*)malloc(sz);
	}
	task->sz = sz;
	memcpy(task->buf, buffer, sz);
}

static inline void
_buf_reserve(struct dtex_loader* dtex, uint32_t sz) {
	if (sz <= dtex->buf_size) {
		return;
	}

	int new_sz = sz * 1.5f;
	unsigned char* buf = malloc(new_sz);
	if (buf == NULL) {
		return;
	}
	dtex->buf_size = new_sz;
	free(dtex->buf);
	dtex->buf = buf;

	pf_log("dtex_loader buf size:%0.1fM\n", (float)new_sz / 1024 / 1024);
}

static inline int 
_check_block_count(struct FileHandle* file) {
	int count = 0;
	for (;;) {
		int32_t sz = 0;
		if (pf_fileread(file, &sz, sizeof(sz)) == 0) {
			return count;
		}
		++count;
		if (sz < 0) {
			sz = -sz;
		}
		pf_fileseek_from_cur(file, sz);
	}
}

#define LZMA_PROPS_SIZE 5

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
_unpack_file(struct dtex_loader* dtex, struct FileHandle* file, void (*unpack_func)(), void* ud) {
	int block_count = _check_block_count(file);
	pf_fileseek_from_head(file, 0);

	for (int i = 0; i < block_count; ++i) {
		int32_t sz = 0;
		pf_fileread(file, &sz, sizeof(sz));
		if (sz < 0) {
			sz = -sz;
			_buf_reserve(dtex, sz);
			if (pf_fileread(file, dtex->buf, sz) != 1) {
				fault("Invalid uncompress data source\n");
			}
			unpack_func(dtex->buf, sz, ud);
		} else {
			uint8_t ori_sz_arr[4];
			pf_fileread(file, ori_sz_arr, sizeof(ori_sz_arr));
			pf_fileseek_from_cur(file, -sizeof(ori_sz_arr));
			size_t ori_sz = ori_sz_arr[0] << 24 | ori_sz_arr[1] << 16 | ori_sz_arr[2] << 8 | ori_sz_arr[3];
			size_t need_sz = sz + 7 + ori_sz;
			_buf_reserve(dtex, need_sz);

			struct block* block = (struct block*)dtex->buf;
			if (sz <= 4 + LZMA_PROPS_SIZE || pf_fileread(file, block, sz) != 1) {
				fault("Invalid compress data source\n");
			}

			unsigned char* buffer = (unsigned char*)(dtex->buf + ((sz + 3) & ~3));
			size_t compressed_sz = sz - sizeof(block->size) - LZMA_PROPS_SIZE;
			int r = _lzma_uncompress(buffer, &ori_sz, block->data, &compressed_sz, block->prop, LZMA_PROPS_SIZE);
			if (r != SZ_OK) {
				fault("Uncompress error %d\n",r);
			}
			unpack_func(buffer, ori_sz, ud);		
		}
	}
}

static inline struct dtex_package* 
_find_package(struct dtex_loader* dtex, const char* name) {
	for (int i = 0; i < dtex->pkg_size; ++i) {
		struct dtex_package* pkg = dtex->packages[i];
		if (strcmp(name, pkg->name) == 0) {
			return pkg;
		}
	}
	return NULL;
}

static inline struct dtex_package*
_new_package(struct dtex_loader* dtex, const char* name) {
	struct dtex_package* pkg = (struct dtex_package*)malloc(sizeof(struct dtex_package));
	pkg->name = _strdup(name);
	pkg->filepath = NULL;
	pkg->tex_size = 0;
	pkg->tex_scale = 1;
	pkg->ej_pkg = NULL;
	pkg->rrp_pkg = NULL;
	pkg->pts_pkg = NULL;

	if (dtex->pkg_size >= PACKAGE_SIZE) {
		fault("dtex->pack_size >= PACKAGE_SIZE\n");
	}		
	dtex->packages[dtex->pkg_size++] = pkg;
	return pkg;
}

struct dtex_package* 
dtexloader_preload_package(struct dtex_loader* dtex, const char* name, const char* path) {
	// open file
	struct FileHandle* file = pf_fileopen(path, "rb");
	if (file == NULL) {
		fault("Can't open name: %s file: %s\n", name, path);
	}

	struct dtex_package* pkg = _find_package(dtex, name);
	if (pkg == NULL) {
		pkg = _new_package(dtex, name);
	}

	int old_tex_size = pkg->tex_size;

	struct unpack2pkg_params params;
	params.pkg = pkg;
	params.load_tex_idx = -1;
	_unpack_file(dtex, file, &_unpack_memory_to_pkg, &params);

	pf_fileclose(file);

	// if load epp
	if (pkg->tex_size > old_tex_size && pkg->filepath == NULL) {
		pkg->filepath = _strdup(path);
	}

	return pkg;
}

static inline void
_unload_texture(struct dtex_raw_tex* tex) {
	glDeleteTextures(1, &tex->id); tex->id = 0;
	if (tex->id_alpha != 0) {
		glDeleteTextures(1, &tex->id_alpha); tex->id_alpha = 0;		
	}
}

static inline void 
_unload_package(struct dtex_package* pkg) {
	for (int i = 0; i < pkg->tex_size; ++i) {
		_unload_texture(&pkg->textures[i]);
	}
}

struct dtex_raw_tex* 
dtexloader_load_texture(struct dtex_loader* dtex, struct dtex_package* pkg, int tex_idx) {
	static struct dtex_package* last_pack = NULL;
	assert(pkg != NULL && tex_idx < pkg->tex_size && tex_idx >= 0);
	struct dtex_raw_tex* tex = &pkg->textures[tex_idx];
	if (tex->id == 0) {
		if (pkg != last_pack && last_pack != NULL) {
			_unload_package(last_pack);
			last_pack = NULL;
		}
		struct FileHandle* file = pf_fileopen(pkg->filepath, "rb");
		if (file == NULL) {
			fault("Can't open name: %s file: %s\n", pkg->name, pkg->filepath);
		}
		struct unpack2pkg_params params;
		params.pkg = pkg;
		params.load_tex_idx = 0;
		_unpack_file(dtex, file, &_unpack_memory_to_pkg, &params);		
		last_pack = pkg;
	}
	assert(tex->id != 0);
	return tex;	
}

void 
dtexloader_unload_texture(struct dtex_raw_tex* tex) {
	_release_texture(tex);
}

struct dtex_package* 
dtexloader_get_package(struct dtex_loader* dtex, int idx) {
	if (idx < 0 || idx >= dtex->pkg_size) {
		return NULL;
	} else {
		return dtex->packages[idx];
	}
}

// todo if already exists
void 
dtexloader_load_spr2task(struct dtex_loader* dtex, struct ej_package* pkg, struct dtex_rect* rect, int id, const char* path) {
	struct FileHandle* file = pf_fileopen(path, "rb");
	if (file == NULL) {
		fault("[dtexloader_load_spr2task] Can't open file: %s\n", path);
	}
	struct unpack2task_params params;
	params.dtex = dtex;
	params.is_epp = true;	
	params.pkg = pkg;
	params.rect = rect;
	params.spr_id = id;
	params.tex_idx = 0;
	_unpack_file(dtex, file, &_unpack_memory_to_task, &params);
	pf_fileclose(file);
}

// to c2
void 
dtexloader_do_task(struct dtex_loader* dtex, void (*on_load_func)()) {
	for (int i = 0; i < dtex->task_size; ++i) {
		struct load_task* task = dtex->tasks[i];
		// load task
		_do_load_task(task);		
		// parser task data
		on_load_func(task->pkg, task->rect, task->spr_id, task->tex_idx, &task->texture);
		// release task
		free(task->buf); task->buf = NULL;
		task->sz = 0;
	}
}

void 
dtexloader_after_do_task(struct dtex_loader* dtex, void (*after_load_func)()) {
	for (int i = 0; i < dtex->task_size; ++i) {
		struct load_task* task = dtex->tasks[i];
		after_load_func(task->pkg, task->rect, task->spr_id, task->tex_idx, &task->texture);
		_release_texture(&task->texture);
	}
	dtex->task_size = 0;
}

struct dtex_rrp* 
dtexloader_query_rrp(struct dtex_loader* dtex, struct ej_package* ej_pkg) {
    if (!dtex) {
        return NULL;
    }
    
	static struct dtex_package* cache_pkg = NULL;
	if (cache_pkg && cache_pkg->ej_pkg == ej_pkg) {
		return cache_pkg->rrp_pkg;
	}

	for (int i = 0; i < dtex->pkg_size; ++i) {
		struct dtex_package* pkg = dtex->packages[i];
		// todo
		// if (pkg->ej_pkg == ej_pkg) {
			cache_pkg = pkg;
			return pkg->rrp_pkg;
		// }
	}
	return NULL;
}

struct dtex_pts* 
dtexloader_query_pts(struct dtex_loader* dtex, struct ej_package* ej_pkg) {
	static struct dtex_package* cache_pkg = NULL;
	if (cache_pkg && cache_pkg->ej_pkg == ej_pkg) {
		return cache_pkg->pts_pkg;
	}

	for (int i = 0; i < dtex->pkg_size; ++i) {
		struct dtex_package* pkg = dtex->packages[i];
		// todo
		// if (pkg->ej_pkg == ej_pkg) {
			cache_pkg = pkg;
			return pkg->pts_pkg;
		// }
	}
	return NULL;
}