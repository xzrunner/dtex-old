#include "dtex_loader.h"
#include "dtex_desc_loader.h"
#include "dtex_texture_loader.h"
#include "dtex_stream_import.h"
#include "dtex_package.h"
#include "dtex_typedef.h"
#include "dtex_texture.h"
#include "dtex_ej_utility.h"
#include "dtex_c2_strategy.h"
#include "dtex_res_path.h"
#include "dtex_png.h"
#include "dtex_gl.h"

#include <fs_file.h>
#include <logger.h>
#include <fault.h>

#ifndef EASY_EDITOR
#include <LzAlloc.h>
#else
#include <Alloc.h>
#endif // EASY_EDITOR
#include <LzmaDec.h>

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define PACKAGE_SIZE 512
#define UNUSED(x)	((void)(x))

struct dtex_loader {
	struct dtex_package packages[PACKAGE_SIZE];	// todo malloc
	int pkg_size;

	// cache memory buf
	char* buf;
	size_t buf_size;
	// cache tex id
	// todo

	int next_pkg_version;
};

struct dtex_loader* 
dtexloader_create() {
	size_t sz = sizeof(struct dtex_loader);
	struct dtex_loader* loader = (struct dtex_loader*)malloc(sz);
	memset(loader, 0, sz);
	loader->next_pkg_version = 1;
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

static void *SzAlloc(void *p, size_t size) { UNUSED(p); return MyAlloc(size); }
static void SzFree(void *p, void *address) { UNUSED(p); MyFree(address); }
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

	if(loader->buf) { free(loader->buf); }
	loader->buf = malloc(sz);
	if (loader->buf == NULL) {
        fault("_buf_reserve malloc failed.\n");
	}
	loader->buf_size = sz;

	LOGI("dtex_loader buf size:%0.1fM", (float)sz / 1024 / 1024);
}

static inline void
_unpack_file(struct dtex_loader* loader, struct fs_file* file, void (*unpack_func)(struct dtex_import_stream* is, void* ud), void* ud) {
	int32_t sz = 0;
	fs_read(file, &sz, sizeof(sz));
	if (sz < 0) {
		sz = -sz;
		char* buf = NULL;
		if (loader) {
			_buf_reserve(loader, sz);
			buf = (char*)loader->buf;
		} else {
			buf = (char*)malloc(sz);
			if (!buf) {
				fault("_unpack_file malloc fail.\n");
			}
		}
		if (fs_read(file, (char*)buf, sz) != sz) {
			if (!loader) { free(buf); }
			fault("Invalid uncompress data source\n");
		}

		struct dtex_import_stream is;
		is.stream = buf;
		is.size = sz;
		unpack_func(&is, ud);

		if (!loader) { free(buf); }
	} else {
		uint8_t ori_sz_arr[4];
		fs_read(file, ori_sz_arr, sizeof(ori_sz_arr));
		fs_seek_from_cur(file, -(int)sizeof(ori_sz_arr));
		size_t ori_sz = ori_sz_arr[0] << 24 | ori_sz_arr[1] << 16 | ori_sz_arr[2] << 8 | ori_sz_arr[3];
		size_t need_sz = sz + 7 + ori_sz;

		char* buf = NULL;
		if (loader) {
			_buf_reserve(loader, need_sz);
			buf = (char*)loader->buf;
		} else {
			buf = (char*)malloc(need_sz);
			if (!buf) {
				fault("_unpack_file malloc fail.\n");
			}
		}

		struct block* block = (struct block*)buf;
		if (sz <= 4 + LZMA_PROPS_SIZE || fs_read(file, block, sz) != sz) {
			if (!loader) { free(buf); }
			fault("Invalid compress data source\n");
		}

		unsigned char* buffer = (unsigned char*)(buf + ((sz + 3) & ~3));
		size_t compressed_sz = sz - sizeof(block->size) - LZMA_PROPS_SIZE;

		int r = _lzma_uncompress(buffer, &ori_sz, block->data, &compressed_sz, block->prop, LZMA_PROPS_SIZE);
		if (r != SZ_OK) {
			if (!loader) { free(buf); }
			fault("Uncompress error %d\n",r);
		}

		struct dtex_import_stream is;
		is.stream = (const char*)buffer;
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
};

struct unpack_tex_raw_params {
	struct dtex_texture* tex;
};

struct unpack_alltex_params {
	struct dtex_package* pkg;
	float scale;
	int size;
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
		// pkg->rrp_pkg = dtex_load_rrp(is);
		break;
	case FILE_PTS:
		// pkg->pts_pkg = dtex_load_pts(is);
		break;
	case FILE_RRR:
		// pkg->rrr_pkg = dtex_load_rrr(is);
		break;
	case FILE_B4R:
		// pkg->b4r_pkg = dtex_load_b4r(is);
		break;
	default:
		fault("_unpack_memory_to_pkg unknown file type %d", params->file_format);
	}
}

static inline void
_unpack_memory_to_texture(struct dtex_import_stream* is, void* ud) {
	struct unpack_tex_params* params = (struct unpack_tex_params*)ud;
	struct dtex_package* pkg = params->pkg;

	assert(params->load_tex_idx < pkg->texture_count);
	struct dtex_texture* tex = pkg->textures[params->load_tex_idx];
	assert(tex);
	dtex_load_texture_all(is, tex);
}

static inline void
_unpack_memory_to_texture_raw(struct dtex_import_stream* is, void* ud) {
	struct unpack_tex_raw_params* params = (struct unpack_tex_raw_params*)ud;
	dtex_load_texture_all(is, params->tex);
}

static inline void
_unpack_memory_to_preload_all_textures(struct dtex_import_stream* is, void* ud) {
	struct unpack_alltex_params* params = (struct unpack_alltex_params*)ud;
	params->size = dtex_load_all_textures_desc(is, params->pkg, params->scale);

	for (int i = 0; i < params->size; ++i) {
		struct relocate_quad_texid_params relocate_params;
		relocate_params.from = i;
		relocate_params.to = params->pkg->textures[i]->uid;
		dtex_ej_pkg_traverse(params->pkg->ej_pkg, _relocate_quad_texid, &relocate_params);
	}
}

static inline void
_unpack_memory_to_preload_texture(struct dtex_import_stream* is, void* ud) {
	struct unpack_pretex_params* params = (struct unpack_pretex_params*)ud;

	struct dtex_package* pkg = params->pkg;

	struct dtex_texture* tex = dtex_texture_create_raw(pkg->LOD);
	if (!tex) {
		fault("_unpack_memory_to_preload_texture dtex_texture_create_raw err.");
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
		if (pkg->name && strcmp(name, pkg->name) == 0) {
			return pkg;
		}
	}
	return NULL;
}

static inline struct dtex_package*
_new_package(struct dtex_loader* loader, const char* name) {
	if (loader->pkg_size >= PACKAGE_SIZE) {
		fault("_new_package: loader->pack_size >= PACKAGE_SIZE\n");
	}

	int id = -1;
	struct dtex_package* pkg = NULL;
	for (int i = 0; i < loader->pkg_size; ++i) {
		if (!loader->packages[i].name) {
			id = i;
			pkg = &loader->packages[i];
			break;
		}
	}
	if (!pkg) {
		id = loader->pkg_size;
		pkg = &loader->packages[loader->pkg_size++];
	}
	memset(pkg, 0, sizeof(*pkg));

	pkg->name = (char*)malloc(strlen(name) + 1);
	strcpy(pkg->name, name);
	pkg->name[strlen(pkg->name)] = 0;

	assert(id != -1);
	pkg->id = id;

	pkg->version = loader->next_pkg_version++;

	return pkg;
}

struct dtex_package* 
dtex_load_pkg(struct dtex_loader* loader, const char* name, const char* epe_path, float scale, int lod) {
	struct dtex_package* pkg = _find_package(loader, name);
	if (!pkg) {
		pkg = _new_package(loader, name);
	}

	pkg->LOD = lod;

	struct fs_file* file = fs_open(epe_path, "rb");
	if (!file) {
		LOGW("dtexloader_preload_pkg: can't open file %s\n", epe_path);
		return NULL;
	}

	struct unpack_pkg_params params;
	params.pkg = pkg;
	params.file_format = FILE_EPE;
	params.scale = scale;
	_unpack_file(loader, file, &_unpack_memory_to_pkg, &params);

	fs_close(file);

	return pkg;
}

void 
dtex_unload_pkg(struct dtex_loader* loader, struct dtex_package* pkg) {
	int idx = -1;
	for (int i = 0; i < loader->pkg_size; ++i) {
		if (&loader->packages[i] == pkg) {
			idx = i;
			break;
		}
	}
	assert(idx != -1);
	dtex_package_release(&loader->packages[idx]);
}

int 
dtex_preload_all_textures(const char* filepath, struct dtex_loader* loader, struct dtex_package* pkg, float scale) {
	struct fs_file* file = fs_open(filepath, "rb");
	if (!file) {
		fault("dtex_preload_all_textures: can't open file %s\n", filepath);
	}

	struct unpack_alltex_params params;
	params.pkg = pkg;
	params.scale = scale;
	params.size = 0;
	_unpack_file(loader, file, &_unpack_memory_to_preload_all_textures, &params);

	fs_close(file);

	return params.size;
}

void 
dtex_preload_texture(struct dtex_loader* loader, struct dtex_package* pkg, int idx, float scale) {
	const char* path = dtex_get_img_filepath(pkg->rp, idx, pkg->LOD);
	struct fs_file* file = fs_open(path, "rb");
	if (!file) {
		fault("dtex_preload_texture: can't open file %s\n", path);
	}

	struct unpack_pretex_params params;
	params.pkg = pkg;
	params.scale = scale;
	_unpack_file(loader, file, &_unpack_memory_to_preload_texture, &params);

	fs_close(file);
}

void 
dtex_load_texture(struct dtex_loader* loader, struct dtex_package* pkg, int idx) {
	const char* path = dtex_get_img_filepath(pkg->rp, idx, pkg->LOD);
	struct fs_file* file = fs_open(path, "rb");
	if (!file) {
		fault("dtex_load_texture: can't open file %s\n", path);
	}

	struct unpack_tex_params params;
	params.pkg = pkg;
	params.load_tex_idx = idx;
	_unpack_file(loader, file, &_unpack_memory_to_texture, &params);

	fs_close(file);
}

void 
dtex_load_texture_raw(struct dtex_loader* loader, const char* path, struct dtex_texture* tex) {
	struct fs_file* file = fs_open(path, "rb");
	if (!file) {
		fault("dtex_load_texture_raw: can't open file %s\n", path);
	}

	struct unpack_tex_raw_params params;
	params.tex = tex;

	_unpack_file(loader, file, &_unpack_memory_to_texture_raw, &params);

	fs_close(file);
}

void 
dtex_load_file(const char* filepath, void (*unpack_func)(struct dtex_import_stream* is, void* ud), void* ud) {
	struct fs_file* file = fs_open(filepath, "rb");
	if (!file) {
		fault("dtexloader_preload_pkg: can't open file %s\n", filepath);
	}

	_unpack_file(NULL, file, unpack_func, ud);

	fs_close(file);
}

static void 
_pre_muilti_alpha(uint8_t* pixels, int width, int height) {
	int pos = 0;
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			float alpha = pixels[pos + 3] / 255.0f;
			for (int i = 0; i < 3; ++i) {
				pixels[pos + i] = (uint8_t)(pixels[pos + i] * alpha);
			}
			pos += 4;
		}
	}
}

struct dtex_texture* 
dtex_load_image(const char* filepath) {
	struct dtex_texture* tex = NULL;
	if (strstr(filepath, ".png")) {
		int w, h, c, f;
		uint8_t* p = dtex_png_read(filepath, &w, &h, &c, &f);
		if ( !p ) {
			return NULL;
		}
	
		_pre_muilti_alpha(p, w, h);
		
		int id = dtex_gl_create_texture(DTEX_TF_RGBA8, w, h, p, c, 0);
		free(p);

		tex = dtex_texture_create_raw(0);
		tex->t.RAW.format = DTEX_PNG8;
		tex->width = w;
		tex->height = h;
		tex->inv_width = 1.0f / w;
		tex->inv_height= 1.0f / h;
		tex->id = id;
	}
	return tex;
}

void 
dtex_package_traverse(struct dtex_loader* loader, void (*pkg_func)(struct dtex_package* pkg)) {
	for (int i = 0; i < loader->pkg_size; ++i) {
		pkg_func(&loader->packages[i]);
	}
}

struct dtex_package* 
dtex_query_pkg(struct dtex_loader* loader, const char* name) {
	for (int i = 0; i < loader->pkg_size; ++i) {
		struct dtex_package* pkg = &loader->packages[i];
		if (pkg->name && strcmp(name, pkg->name) == 0) {
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

int 
dtex_load_buf_sz(struct dtex_loader* loader) {
	return loader->buf_size;
}