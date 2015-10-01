#include "dtex_facade.h"
#include "dtex_loader_new.h"
#include "dtex_c3.h"
#include "dtex_c2.h"
#include "dtex_c1.h"
#include "dtex_c1_new.h"
#include "dtex_buffer.h"
#include "dtex_async.h"
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
#include "dtex_utility2.h"
#include "dtex_relocation.h"
#include "dtex_texture.h"
#include "dtex_array.h"

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
static struct dtex_buffer*	BUF = NULL;

/************************************************************************/
/* dtexf overall                                                        */
/************************************************************************/

struct dtex_config {
	bool open_c1;
	bool open_c2;
	bool open_c3;
};
struct dtex_config CFG;

static inline void 
_config(const char* str) {
	cJSON* root = cJSON_Parse(str);
	if (!root) {
		dtex_info("dtex parse config fail!\n");
	}

	CFG.open_c1 = cJSON_GetObjectItem(root, "open_c1")->valueint;
	CFG.open_c2 = cJSON_GetObjectItem(root, "open_c2")->valueint;
	CFG.open_c3 = cJSON_GetObjectItem(root, "open_c3")->valueint;

	cJSON_Delete(root);
}

void 
dtexf_create(const char* cfg) {
	dtex_stat_init();

	CFG.open_c1 = true;
	CFG.open_c2 = true;
	CFG.open_c3 = true;
	if (cfg) {
		_config(cfg);		
	}

	dtex_async_loader_init();

	dtex_texture_pool_init();

	LOADER = dtexloader_create();

	BUF = dtexbuf_create();
	if (CFG.open_c3) {
		C3 = dtex_c3_create();	
	}
 	if (CFG.open_c1) {
 		C1 = dtex_c1_create(BUF);		
 	}
 	if (CFG.open_c2) {
 		C2 = dtex_c2_create(BUF);		
 	}
}

void 
dtexf_release() {
 	if (C2) {
 		dtex_c2_release(C2, BUF);		
 	}
	if (C1) {
		dtex_c1_release(C1, BUF);		
	}
	if (C3) {
		dtex_c3_release(C3, BUF);		
	}
	dtexbuf_release(BUF);
	if (LOADER) {
		dtexloader_release(LOADER);		
	}

	dtex_async_loader_release();
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
dtexf_preload_pkg(const char* name, const char* path, int type, float scale) {
	return dtex_preload_pkg(LOADER, name, path, type, scale);
}

void 
dtexf_load_texture(struct dtex_package* pkg, int idx, float scale) {
	dtex_load_texture(LOADER, BUF, pkg, idx, scale);
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
//	struct dp_pos* pos = dtex_c3_load_tex(C3, src_tex, BUF, &dst_tex);
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
		dtex_c3_load_end(C3, LOADER, BUF, async);
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
		dtex_c2_load_end(C2, BUF, LOADER, true);
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

//void 
//dtexf_async_load_spr(const char* pkg_name, const char* spr_name, const char* path) {
//	struct dtex_package* pkg = dtex_c3_query_pkg(C3, pkg_name);
//
//	struct dtex_rect* rect[pkg->tex_size];
//	dtex_c3_query_rect(C3, pkg_name, rect, pkg->tex_size);
//
//	int spr_id = sprite_id(pkg->ej_pkg, spr_name);
//	dtex_async_load_spr(LOADER, pkg->ej_pkg, rect, pkg->tex_size, spr_id, path);
//}

//static inline void
//_on_load_spr_task(struct ej_sprite_pack* ej_pkg, struct dtex_rect* rect, int spr_id, int tex_idx, struct dtex_texture* dst_tex) {
//	struct dtex_img_pos ori_pos, dst_pos;
//	_prepare_trans_pos(rect, tex_idx, dst_tex, &ori_pos, &dst_pos);
//
//	struct int_array* array = dtex_get_picture_id_set(ej_pkg, spr_id);
//
//	dtex_relocate_spr(ej_pkg, array, tex_idx, &ori_pos, &dst_pos);
//	dtex_c2_load(C2, ej_pkg, spr_id, tex_idx);
//	dtex_relocate_spr(ej_pkg, array, tex_idx, &dst_pos, &ori_pos);
//
//	free(array);
//}

//static inline void
//_after_load_spr_task(struct ej_package* pkg, struct dtex_rect* rect, int spr_id, int tex_idx, struct dtex_texture* dst_tex) {
//	struct dtex_img_pos ori_pos, dst_pos;
//	_prepare_trans_pos(rect, tex_idx, dst_tex, &ori_pos, &dst_pos);
//
//	struct int_array* array = dtex_get_picture_id_set(pkg, spr_id);
//
//    dtex_relocate_spr(pkg, array, tex_idx, &ori_pos, &dst_pos);
//	dtex_relocate_c2_key(C2, pkg, array, &dst_pos, &ori_pos);
//	dtex_relocate_spr(pkg, array, tex_idx, &dst_pos, &ori_pos);
//
//	free(array);
//}
//
//static inline void
//_do_load_task() {
//	if (!dtexloader_has_task(LOADER)) {
//		return;
//	}
//
//	dtexc2_preload_begin(C2);	
//	dtexloader_do_task(LOADER, &_on_load_spr_task);
//	dtexc2_preload_end(C2, BUF, LOADER, true);
//	dtexloader_after_do_task(LOADER, &_after_load_spr_task);	
//}
//

/************************************************************************/
/* async load texture                                                   */
/************************************************************************/

void 
dtexf_async_load_texture(struct dtex_package* pkg, int idx) {
	dtex_async_load_texture(BUF, pkg, idx);
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

	dtex_async_load_multi_textures(BUF, pkg, tex_idx, _async_load_texture_from_c3_func, params);
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
	dtex_c2_load_end(C2, BUF, LOADER, true);
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

//	dtex_async_load_multi_textures(BUF, pkg, uids, count, cb, ud);
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
	//	struct dtex_array* picture_ids;
};

static inline void
_async_load_texture_with_c2_from_c3_func(void* ud) {
	if (!C2) {
		return;
	}

	struct async_load_texture_with_c2_from_c3_params* params = (struct async_load_texture_with_c2_from_c3_params*)ud;

	struct dtex_package* pkg = params->pkg;

	struct dtex_array* pictures = dtex_get_picture_id_unique_set(pkg->ej_pkg, params->sprite_ids, params->sprite_count);
	struct dtex_array* textures = dtex_get_texture_id_unique_set(pkg->ej_pkg, params->sprite_ids, params->sprite_count);
	int tex_size = dtex_array_size(textures);

	struct dtex_texture* c3_textures[pkg->texture_count];
	memset(c3_textures, 0, sizeof(c3_textures));
	struct dtex_rect* c3_regions[pkg->texture_count];
	memset(c3_regions, 0, sizeof(c3_regions));
	dtex_c3_query_map_info(C3, pkg, c3_textures, c3_regions);

	for (int i = 0; i < tex_size; ++i) {
		int idx = *(int*)dtex_array_fetch(textures, i);
		struct dtex_img_pos ori_pos, dst_pos;
		dtex_prepare_c3_trans_pos(c3_regions[idx], c3_textures[idx], pkg->textures[idx], &ori_pos, &dst_pos);
		dtex_relocate_spr(pkg, idx, pictures, &ori_pos, &dst_pos);
	}

	dtex_c2_load_begin(C2); 
	for (int i = 0; i < params->sprite_count; ++i) {
		dtexf_c2_load(pkg, params->sprite_ids[i]);
	}
	dtex_c2_load_end(C2, BUF, LOADER, true);

	// 	for (int i = 0; i < tex_size; ++i) {
	// 		struct dtex_img_pos ori_pos, dst_pos;
	// 		dtex_prepare_c3_trans_pos(rect, );
	// 		dtex_relocate_c2_key();
	// 		dtex_relocate_spr(&dst_pos, &ori_pos);
	// 	}

	dtex_array_release(textures);
	dtex_array_release(pictures);

	free(params);
}

void 
dtexf_async_load_texture_with_c2_from_c3(struct dtex_package* pkg, int* sprite_ids, int sprite_count) {
	struct async_load_texture_with_c2_from_c3_params* params = (struct async_load_texture_with_c2_from_c3_params*)malloc(sizeof(*params));
	params->pkg = pkg;
//	params->picture_ids = dtex_get_picture_id_unique_set(pkg->ej_pkg, sprite_ids, sprite_count);
	params->sprite_ids = sprite_ids;
	params->sprite_count = sprite_count;
	
	struct dtex_array* picture_ids = dtex_get_picture_id_unique_set(pkg->ej_pkg, sprite_ids, sprite_count);
	dtex_swap_quad_src_info(pkg, picture_ids);
	_async_load_texture_with_c2(pkg, sprite_ids, sprite_count, _async_load_texture_with_c2_from_c3_func, params);
	dtex_swap_quad_src_info(pkg, picture_ids);
}

/************************************************************************/
/* update for async loading                                             */
/************************************************************************/

void 
dtexf_update() {
	dtex_async_loader_update(BUF);
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
//	tex = dtex_gl_create_texture(TEXTURE_PVR4, width, height, new_compressed, 0);
//	free(new_compressed);
//#else
//	tex = dtex_gl_create_texture(TEXTURE_RGBA8, width, height, buf_uncompressed, 0);
//#endif
//	free(buf_uncompressed);
//
//	struct dtex_texture src_tex;
//	src_tex.id = tex;
//	src_tex.width = width;
//	src_tex.height = height;
//	src_tex.type = TT_RAW;
//	src_tex.t.RAW.format = TEXTURE8;
//	src_tex.t.RAW.id_alpha = 0;
//
//	struct dtex_texture* dst_tex = NULL;
//	dtex_c3_load_tex(C3, &src_tex, BUF, &dst_tex);
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
//	tex = dtex_gl_create_texture(TEXTURE_ETC1, width, height, new_compressed, 0);
//	free(new_compressed);
//#else
//	tex = dtex_gl_create_texture(TEXTURE_RGBA8, width, height, buf_uncompressed, 0);
//#endif
//	free(buf_uncompressed);
//
//	struct dtex_texture src_tex;
//	src_tex.id = tex;
//	src_tex.width = width;
//	src_tex.height = height;
//	src_tex.type = TT_RAW;
//	src_tex.t.RAW.format = TEXTURE8;
//	src_tex.t.RAW.id_alpha = 0;
//
//	struct dtex_texture* dst_tex = NULL;
//	dtex_c3_load_tex(C3, &src_tex, BUF, &dst_tex);
//}
//#endif