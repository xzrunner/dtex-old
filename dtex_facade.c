#include "dtex_facade.h"
#include "dtex_loader.h"
#include "dtex_c3.h"
#include "dtex_c2.h"
#include "dtex_c1.h"
#include "dtex_c1_new.h"
#include "dtex_rrp.h"
#include "dtex_pts.h"
#include "dtex_draw.h"
#include "dtex_pvr.h"
#include "dtex_etc1.h"
#include "dtex_packer.h"
#include "dtex_sprite.h"
#include "dtex_gl.h"
#include "dtex_file.h"
#include "dtex_log.h"
#include "dtex_package.h"
#include "dtex_statistics.h"
#include "dtex_ej_sprite.h"
#include "dtex_async_loader.h"
#include "dtex_async_task.h"
#include "dtex_array.h"
#include "dtex_utility.h"
#include "dtex_relocation.h"
#include "dtex_texture.h"
#include "dtex_array.h"
#include "dtex_hard_res.h"
#include "dtex_res_cache.h"
#include "dtex_resource.h"

#include <cJSON.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define MAX_PACKAGE 512

static struct dtex_loader* LOADER = NULL;
static struct dtex_c3* C3 = NULL;
static struct dtex_c2* C2 = NULL;
static struct dtex_c1* C1 = NULL;

/************************************************************************/
/* dtexf overall                                                        */
/************************************************************************/

struct dtex_config {
	int needed_texture;

	bool open_c1;
	bool open_c2;
	bool open_c3;

	int c1_tex_size;
	int c2_tex_size;
	int c3_tex_size;

	int LOD[3];
};
struct dtex_config CFG;

static inline void 
_config(const char* str) {
	cJSON* root = cJSON_Parse(str);
	if (!root) {
		dtex_info("dtex parse config fail!\n");
	}

	if (cJSON_GetObjectItem(root, "needed_texture")) {
		CFG.needed_texture = cJSON_GetObjectItem(root, "needed_texture")->valueint;
	}

	CFG.open_c1 = cJSON_GetObjectItem(root, "open_c1")->valueint;
	CFG.open_c2 = cJSON_GetObjectItem(root, "open_c2")->valueint;
	CFG.open_c3 = cJSON_GetObjectItem(root, "open_c3")->valueint;

	if (cJSON_GetObjectItem(root, "c1_tex_size")) {
		CFG.c1_tex_size = cJSON_GetObjectItem(root, "c1_tex_size")->valueint;
	}
	if (cJSON_GetObjectItem(root, "c2_tex_size")) {
		CFG.c2_tex_size = cJSON_GetObjectItem(root, "c2_tex_size")->valueint;
	}
	if (cJSON_GetObjectItem(root, "c3_tex_size")) {
		CFG.c3_tex_size = cJSON_GetObjectItem(root, "c3_tex_size")->valueint;
	}

	cJSON* lod = cJSON_GetObjectItem(root, "LOD");
	if (lod) {
		int lod_sz = cJSON_GetArraySize(lod);
		for (int i = 0; i < lod_sz && i < 3; ++i) {
			CFG.LOD[i] = cJSON_GetArrayItem(lod, i)->valueint;
		}
	}

	cJSON_Delete(root);
}

void 
dtexf_create(const char* cfg) {
	CFG.needed_texture = 8;

	CFG.open_c1 = true;
	CFG.open_c2 = true;
	CFG.open_c3 = true;

	CFG.c1_tex_size = 1024;
	CFG.c2_tex_size = 4096;
	CFG.c3_tex_size = 2048;

	CFG.LOD[0] = 100;
	CFG.LOD[1] = 50;
	CFG.LOD[2] = 25;

	if (cfg) {
		_config(cfg);		
	}

	dtex_hard_res_init(CFG.needed_texture * 2048 * 2048);

	dtex_stat_init();

	dtex_lod_init(CFG.LOD);

	dtex_res_cache_create();

	dtex_async_loader_init();

	dtex_texture_pool_init();

	LOADER = dtexloader_create();

	if (CFG.open_c3) {
		C3 = dtex_c3_create(CFG.c3_tex_size);	
	}
 	if (CFG.open_c1) {
 		C1 = dtex_c1_create(CFG.c1_tex_size);		
 	}
 	if (CFG.open_c2) {
 		C2 = dtex_c2_create(CFG.c2_tex_size);		
 	}
}

void 
dtexf_release() {
 	if (C2) {
 		dtex_c2_release(C2);		
 	}
	if (C1) {
		dtex_c1_release(C1);		
	}
	if (C3) {
		dtex_c3_release(C3);		
	}
	if (LOADER) {
		dtexloader_release(LOADER);		
	}

	dtex_async_loader_release();

	dtex_res_cache_release();
}

struct dtex_package* 
dtexf_query_pkg(const char* name) {
	return dtex_query_pkg(LOADER, name);
}

/************************************************************************/
/* sprite draw                                                          */
/************************************************************************/

void 
dtexf_sprite_draw(struct dtex_package* pkg, struct ej_sprite* spr, struct ej_srt* srt) {
	dtex_ej_sprite_draw(pkg, C2, spr, srt);
}

/************************************************************************/
/* sync load desc file (epe, pts...) and texture                        */
/************************************************************************/

struct dtex_package* 
dtexf_load_pkg(const char* name, const char* path, int type, float scale, int lod) {
	return dtex_load_pkg(LOADER, name, path, type, scale, lod);
}

void 
dtexf_preload_texture(struct dtex_package* pkg, int idx, float scale) {
	dtex_preload_texture(LOADER, pkg, idx, scale);
}

void
dtexf_load_texture(struct dtex_package* pkg, int idx, float scale, bool create_by_ej) {
	dtex_load_texture(LOADER, pkg, idx, scale, create_by_ej);
}

//struct ej_sprite* 
//dtexf_create_sprite(const char* path) {
//	if (C3 == NULL) {
//		return NULL;
//	}
//
//	struct dtex_texture* src_tex = dtexloader_load_image(path);
//
//	struct dtex_texture* dst_tex = NULL;
//	struct dp_pos* pos = dtex_c3_load_tex(C3, src_tex, &dst_tex);
//
//	dtexloader_unload_tex(src_tex);
//	free(src_tex);
//
//	return dtex_sprite_create(dst_tex, pos);
//}

/************************************************************************/
/* C3                                                                   */
/************************************************************************/

void
dtexf_c3_load(struct dtex_package* pkg, float scale) {
	if (C3) {
		dtex_c3_load(C3, pkg, scale);
	}
}

void 
dtexf_c3_load_end(bool async) {
	if (C3) {
		dtex_c3_load_end(C3, LOADER, async);
	}
}

/************************************************************************/
/* C2                                                                   */
/************************************************************************/

void 
dtexf_c2_load_begin() {
	if (C2) {
		dtex_c2_load_begin(C2);
	}
}

void 
dtexf_c2_load(struct dtex_package* pkg, int spr_id) {
	if (C2) {
		dtex_c2_load(C2, pkg, spr_id);		
	}
}

void 
dtexf_c2_load_end() {
	if (C2) {
		dtex_c2_load_end(C2, LOADER, true);
	}
}

float* 
dtexf_c2_lookup_texcoords(struct dtex_texture* ori_tex, float* ori_vb, int* dst_tex) {
	if (C2) {
		return dtex_c2_lookup_texcoords(C2, ori_tex, ori_vb, dst_tex);
	} else {
		return NULL;
	}
}

//void 
//dtexf_c2_lookup_node(struct ej_texture* ori_tex, float* ori_vb, 
//	struct dtex_texture** out_tex, struct dp_pos** out_pos) {
//
//	if (C2 == NULL) {
//		return;
//	}
//
//	struct dtex_rect rect;
//	_get_pic_ori_rect(1/ori_tex->width, 1/ori_tex->height, ori_vb, &rect);
//
//	dtexc2_lookup_node(C2, ori_tex->id, &rect, out_tex, out_pos);
//}

/************************************************************************/
/* C1                                                                   */
/************************************************************************/

void 
dtexf_c1_update(struct dtex_package* pkg, struct ej_sprite* spr) {
	dtex_c1_update(C1, C2, pkg, spr);
}

//void 
//dtexf_c1_load_anim(struct ej_package* pkg, struct animation* ani, int action) {
//	if (C1) {
//		dtex_c1_load_anim(C1, pkg, ani, action);		
//	}
//}
//
//bool 
//dtexf_c1_draw_anim(struct ej_package* pkg, struct animation* ani, int action, 
//	int frame, struct draw_params* params) {
//   if (C1 == NULL) {
//       return false;
//   }
//   frame /= 2;
//	return dtex_c1_draw_anim(C1, pkg, ani, action, frame, params);
//}

/************************************************************************/
/* async load texture                                                   */
/************************************************************************/

void 
dtexf_async_load_texture(struct dtex_package* pkg, int idx) {
	dtex_async_load_texture(pkg, idx, "normal");
}

/************************************************************************/
/* 1. C3 load small scale 2. async load origin size texture             */
/************************************************************************/

struct async_load_texture_from_c3_params {
	struct dtex_package* pkg;
	struct dtex_array* picture_ids;
};

static inline void
_async_load_texture_from_c3_func(void* ud) {
	struct async_load_texture_from_c3_params* params = (struct async_load_texture_from_c3_params*)ud;
	
	dtex_swap_quad_src_info(params->pkg, params->picture_ids);

	dtex_array_release(params->picture_ids);
	free(params);
}

void 
dtexf_async_load_texture_from_c3(struct dtex_package* pkg, int* sprite_ids, int sprite_count) {
	// swap to origin data, get texture idx info
	struct dtex_array* picture_ids = dtex_get_picture_id_unique_set(pkg->ej_pkg, sprite_ids, sprite_count);
	dtex_swap_quad_src_info(pkg, picture_ids);
	struct dtex_array* tex_idx = dtex_get_texture_id_unique_set(pkg->ej_pkg, sprite_ids, sprite_count);
	dtex_swap_quad_src_info(pkg, picture_ids);

	struct async_load_texture_from_c3_params* params = malloc(sizeof(struct async_load_texture_from_c3_params));
	params->pkg = pkg;
	params->picture_ids = picture_ids;

	dtex_async_load_multi_textures(pkg, tex_idx, _async_load_texture_from_c3_func, params, "only c3");
	dtex_array_release(tex_idx);
}

/************************************************************************/
/* 1. normal loading 2. async load needed texture and pack to C2        */
/************************************************************************/

struct async_load_texture_with_c2_params {
	struct dtex_package* pkg;
	int* sprite_ids;
	int sprite_count;
};

static inline void
_async_load_texture_with_c2_func(void* ud) {
	if (!C2) {
		return;
	}

	struct async_load_texture_with_c2_params* params = (struct async_load_texture_with_c2_params*)ud;
	dtex_c2_load_begin(C2);
	for (int i = 0; i < params->sprite_count; ++i) {
		dtexf_c2_load(params->pkg, params->sprite_ids[i]);
	}
	dtex_c2_load_end(C2, LOADER, true);
	free(params);
}

void
_async_load_texture_with_c2(struct dtex_package* pkg, int* sprite_ids, int sprite_count, void (*cb)(void* ud), void* ud) {
	struct dtex_array* tex_idx = dtex_get_texture_id_unique_set(pkg->ej_pkg, sprite_ids, sprite_count);
	int count = dtex_array_size(tex_idx);
	int uids[count];
	for (int i = 0; i < count; ++i) {
		uids[i] = *(int*)dtex_array_fetch(tex_idx, i);
	}
	dtex_array_release(tex_idx);

//	dtex_async_load_multi_textures(pkg, uids, count, cb, ud);
}

void 
dtexf_async_load_texture_with_c2(struct dtex_package* pkg, int* sprite_ids, int sprite_count) {
	struct async_load_texture_with_c2_params* params = (struct async_load_texture_with_c2_params*)malloc(sizeof(*params));
	params->pkg = pkg;
	params->sprite_ids = sprite_ids;
	params->sprite_count = sprite_count;
	_async_load_texture_with_c2(pkg, sprite_ids, sprite_count, _async_load_texture_with_c2_func, params);
}

/************************************************************************/
/* 1. C3 loading 2. async load needed texture and pack to C2            */
/************************************************************************/

struct async_load_texture_with_c2_from_c3_params {
	struct dtex_package* pkg;
	int* sprite_ids;
	int sprite_count;
	struct dtex_array* picture_ids;
	struct dtex_array* tex_idx;
};

static inline void
_async_load_texture_with_c2_from_c3_func(void* ud) {
	struct async_load_texture_with_c2_from_c3_params* params = (struct async_load_texture_with_c2_from_c3_params*)ud;

	if (!C2) {
		dtex_swap_quad_src_info(params->pkg, params->picture_ids);
		dtex_array_release(params->picture_ids);
		dtex_array_release(params->tex_idx);
		free(params);
		return;
	}

	struct dtex_package* pkg = params->pkg;

	dtex_swap_quad_src_info(pkg, params->picture_ids);

	dtex_info("+++++++++++++++ c2 [[[[, %s \n", pkg->name);

	dtex_c2_load_begin(C2); 
	for (int i = 0; i < params->sprite_count; ++i) {
		dtexf_c2_load(pkg, params->sprite_ids[i]);
	}
	free(params->sprite_ids);
	dtex_c2_load_end(C2, LOADER, true);

	dtex_info("+++++++++++++++ c2 ]]]], %s \n", pkg->name);

	struct dtex_texture* dst_textures[pkg->texture_count];
	struct dtex_rect* dst_regions[pkg->texture_count];
	dtex_c3_query_map_info(C3, pkg, dst_textures, dst_regions);

	int sz = dtex_array_size(params->tex_idx);
 	for (int i = 0; i < sz; ++i) {
		int idx = *(int*)dtex_array_fetch(params->tex_idx, i);

		struct dtex_texture *src_tex = pkg->textures[idx], 
			*dst_tex = dst_textures[idx];

		assert(src_tex->type == DTEX_TT_RAW && dst_tex->type == DTEX_TT_MID);
 		struct dtex_texture_with_rect src, dst;

		src.tex = src_tex;
		src.rect.xmin = src.rect.ymin = 0;
		src.rect.xmax = src_tex->width;
		src.rect.ymax = src_tex->height;

		dst.tex = dst_tex;
		dst.rect = *dst_regions[idx];

 		dtex_relocate_c2_key(C2, pkg, idx, params->picture_ids, &src, &dst);
 	}

	dtex_swap_quad_src_info(pkg, params->picture_ids);

	for (int i = 0; i < sz; ++i) {
		int idx = *(int*)dtex_array_fetch(params->tex_idx, i);
		dtex_texture_release(pkg->textures[idx]);
	}
	dtex_package_remove_all_textures_ref(pkg);

	dtex_array_release(params->picture_ids);
	dtex_array_release(params->tex_idx);
	free(params);
}

void 
dtexf_async_load_texture_with_c2_from_c3(struct dtex_package* pkg, int* sprite_ids, int sprite_count) {
	// swap to origin data, get texture idx info
	struct dtex_array* picture_ids = dtex_get_picture_id_unique_set(pkg->ej_pkg, sprite_ids, sprite_count);
	dtex_swap_quad_src_info(pkg, picture_ids);
	struct dtex_array* tex_idx = dtex_get_texture_id_unique_set(pkg->ej_pkg, sprite_ids, sprite_count);
	dtex_swap_quad_src_info(pkg, picture_ids);

	struct async_load_texture_with_c2_from_c3_params* params = (struct async_load_texture_with_c2_from_c3_params*)malloc(sizeof(*params));
	params->pkg = pkg;
	params->sprite_ids = sprite_ids;
	params->sprite_count = sprite_count;
	params->picture_ids = picture_ids;
	params->tex_idx = tex_idx;
	
	dtex_package_change_lod(pkg, 0);
	dtex_async_load_multi_textures(pkg, tex_idx, _async_load_texture_with_c2_from_c3_func, params, "c2 from c3");
}

/************************************************************************/
/* update for async loading                                             */
/************************************************************************/

void 
dtexf_update() {
	dtex_async_loader_update();
}
//
//bool 
//dtexf_draw_rrp(struct ej_package* pkg, struct ej_texture* tex, int id, 
//	struct draw_params* params, const int32_t part_screen[8]) {
//
//	struct dtex_rrp* rrp = dtexloader_query_rrp(LOADER, pkg);
//	if (rrp == NULL) {
//		return false;
//	}
//	struct rrp_picture* pic = dtex_rrp_get_pic(rrp, id);
//	if (pic == NULL) {
//		return false;
//	}
//
//	struct dtex_texture src;
//	src.id = tex->id;
//	src.id_alpha = tex->id_alpha;
//	src.width = 1.0f / tex->width;
//	src.height = 1.0f / tex->height;
//	dtex_draw_rrp(&src, pic, params, part_screen);
//	return true;
//}
//
//bool 
//dtexf_draw_pts(struct ej_package* pkg, struct dtex_texture* src, int src_id, 
//	struct dp_pos* src_pos, struct draw_params* params, const int32_t part_screen[8]) {
//
//	struct dtex_pts* pts = dtexloader_query_pts(LOADER, pkg);
//	if (pts == NULL) {
//		return false;
//	}
//	struct pts_picture* pic = dtex_pts_get_pic(pts, src_id);
//	if (pic == NULL) {
//		return false;
//	}
//
//	dtex_draw_pts(src, src_pos, pic, params, part_screen);
//	return true;
//}

/************************************************************************/
/* debug                                                                */
/************************************************************************/

void 
dtexf_debug_draw() {
  	if (C1) {
  		dtex_c1_debug_draw(C1);
  	} else if (C2) {
		dtex_c2_debug_draw(C2);
	} else if (C3) {
		dtex_c3_debug_draw(C3);
	}

//	dtex_debug_draw(4);
}

//void 
//dtexf_test_pvr(const char* path) {
//	uint32_t width, height;
//	uint8_t* buf_compressed = dtex_pvr_read_file(path, &width, &height);
//	assert(buf_compressed);
//
//	uint8_t* buf_uncompressed = dtex_pvr_decode(buf_compressed, width, height);
//	free(buf_compressed);
//
//	unsigned int tex;
//#ifdef __APPLE__
//	uint8_t* new_compressed = dtex_pvr_encode(buf_uncompressed, width, height);
//	tex = dtex_gl_create_texture(DTEX_TF_PVR4, width, height, new_compressed, 0);
//	free(new_compressed);
//#else
//	tex = dtex_gl_create_texture(DTEX_TF_RGBA8, width, height, buf_uncompressed, 0);
//#endif
//	free(buf_uncompressed);
//
//	struct dtex_texture src_tex;
//	src_tex.id = tex;
//	src_tex.width = width;
//	src_tex.height = height;
//	src_tex.type = DTEX_TT_RAW;
//	src_tex.t.RAW.format = TEXTURE8;
//	src_tex.t.RAW.id_alpha = 0;
//
//	struct dtex_texture* dst_tex = NULL;
//	dtex_c3_load_tex(C3, &src_tex, &dst_tex);
//}
//
//#ifndef __ANDROID__
//
//void 
//dtexf_test_etc1(const char* path) {
//	uint32_t width, height;
//	uint8_t* buf_compressed = dtex_etc1_read_file(path, &width, &height);
//	assert(buf_compressed);
//
//	uint8_t* buf_uncompressed = dtex_etc1_decode(buf_compressed, width, height);
//	free(buf_compressed);
//
//	unsigned int tex;
//#ifdef __ANDROID__
//	uint8_t* new_compressed = dtex_pvr_encode(buf_uncompressed, width, height);
//	tex = dtex_gl_create_texture(DTEX_TF_ETC1, width, height, new_compressed, 0);
//	free(new_compressed);
//#else
//	tex = dtex_gl_create_texture(DTEX_TF_RGBA8, width, height, buf_uncompressed, 0);
//#endif
//	free(buf_uncompressed);
//
//	struct dtex_texture src_tex;
//	src_tex.id = tex;
//	src_tex.width = width;
//	src_tex.height = height;
//	src_tex.type = DTEX_TT_RAW;
//	src_tex.t.RAW.format = TEXTURE8;
//	src_tex.t.RAW.id_alpha = 0;
//
//	struct dtex_texture* dst_tex = NULL;
//	dtex_c3_load_tex(C3, &src_tex, &dst_tex);
//}
//#endif