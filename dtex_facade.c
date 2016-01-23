#include "dtex_facade.h"
#include "dtex_loader.h"
#include "dtex_c4.h"
#include "dtex_c3.h"
#include "dtex_c2.h"
#include "dtex_c1.h"
#include "dtex_rrp.h"
#include "dtex_pts.h"
#include "dtex_pvr.h"
#include "dtex_etc1.h"
#include "dtex_tp.h"
#include "dtex_sprite.h"
#include "dtex_gl.h"
#include "dtex_file.h"
#include "dtex_log.h"
#include "dtex_package.h"
#include "dtex_statistics.h"
#include "dtex_ej_sprite.h"
#include "dtex_async_loader.h"
#include "dtex_array.h"
#include "dtex_utility.h"
#include "dtex_relocation.h"
#include "dtex_texture.h"
#include "dtex_array.h"
#include "dtex_hard_res.h"
#include "dtex_res_cache.h"
#include "dtex_lod.h"
#include "dtex_async_one_tex_task.h"
#include "dtex_async_multi_tex_task.h"
#include "dtex_async_c3_task.h"
#include "dtex_async_c2_task.h"
#include "dtex_async_c2_from_c3_task.h"
#include "dtex_render.h"
#include "dtex_texture_cache.h"
#include "dtex_c2_strategy.h"
#include "dtex_cg.h"
#include "dtex_cs.h"

#include <cJSON.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define MAX_PACKAGE 512

static struct dtex_loader* LOADER = NULL;
static struct dtex_c4* C4 = NULL;
static struct dtex_c3* C3 = NULL;
static struct dtex_c2* C2 = NULL;
static struct dtex_c1* C1 = NULL;
static struct dtex_cs* CS = NULL;

/************************************************************************/
/* dtexf overall                                                        */
/************************************************************************/

struct dtex_config {
	int needed_texture;

	bool open_c1;
	bool open_c2;
	bool open_c3;
	bool open_c4;
	bool open_cg;

	int c1_tex_size;
	int c2_tex_size;
	int c3_tex_size;
	int c4_tex_size;

	int src_extrude;

	int LOD[3];

	int c2_max_no_update_count;
};
struct dtex_config CFG;

static inline void 
_config(const char* str) {
	cJSON* root = cJSON_Parse(str);
	if (!root) {
		dtex_warning("dtex parse config fail!");
	}

	if (cJSON_GetObjectItem(root, "needed_texture")) {
		CFG.needed_texture = cJSON_GetObjectItem(root, "needed_texture")->valueint;
	}

	CFG.open_c1 = cJSON_GetObjectItem(root, "open_c1")->valueint;
	CFG.open_c2 = cJSON_GetObjectItem(root, "open_c2")->valueint;
	CFG.open_c3 = cJSON_GetObjectItem(root, "open_c3")->valueint;
	CFG.open_c4 = cJSON_GetObjectItem(root, "open_c4")->valueint;
	if (cJSON_GetObjectItem(root, "open_cg")) {
		CFG.open_cg = cJSON_GetObjectItem(root, "open_cg")->valueint;
	}

	if (cJSON_GetObjectItem(root, "c1_tex_size")) {
		CFG.c1_tex_size = cJSON_GetObjectItem(root, "c1_tex_size")->valueint;
	}
	if (cJSON_GetObjectItem(root, "c2_tex_size")) {
		CFG.c2_tex_size = cJSON_GetObjectItem(root, "c2_tex_size")->valueint;
	}
	if (cJSON_GetObjectItem(root, "c3_tex_size")) {
		CFG.c3_tex_size = cJSON_GetObjectItem(root, "c3_tex_size")->valueint;
	}
	if (cJSON_GetObjectItem(root, "c4_tex_size")) {
		CFG.c4_tex_size = cJSON_GetObjectItem(root, "c4_tex_size")->valueint;
	}

	if (cJSON_GetObjectItem(root, "src_extrude")) {
		CFG.src_extrude = cJSON_GetObjectItem(root, "src_extrude")->valueint;
	}

	cJSON* lod = cJSON_GetObjectItem(root, "LOD");
	if (lod) {
		int lod_sz = cJSON_GetArraySize(lod);
		for (int i = 0; i < lod_sz && i < 3; ++i) {
			CFG.LOD[i] = cJSON_GetArrayItem(lod, i)->valueint;
		}
	}

	if (cJSON_GetObjectItem(root, "c2_max_no_update_count")) {
		CFG.c2_max_no_update_count = cJSON_GetObjectItem(root, "c2_max_no_update_count")->valueint;
	}

	cJSON_Delete(root);
}

void 
dtexf_create(const char* cfg) {
	CFG.needed_texture = 8;

	CFG.open_c1 = true;
	CFG.open_c2 = true;
	CFG.open_c3 = true;
	CFG.open_c4 = true;
	CFG.open_cg = false;

	CFG.c1_tex_size = 1024;
	CFG.c2_tex_size = 4096;
	CFG.c3_tex_size = 2048;
	CFG.c4_tex_size = 2048;

	CFG.src_extrude = 0;

	CFG.LOD[0] = 100;
	CFG.LOD[1] = 50;
	CFG.LOD[2] = 25;

	CFG.c2_max_no_update_count = 60 * 30;

	if (cfg) {
		_config(cfg);		
	}

	dtex_log_init();

	dtex_hard_res_init(CFG.needed_texture * 2048 * 2048);

	dtex_stat_init();

	dtex_lod_init(CFG.LOD);

	dtex_res_cache_create();

	dtex_texture_cache_init(2048 * 2048 * 2);

	dtex_c2_strategy_init(CFG.c2_max_no_update_count);

	// async load
	dtex_async_loader_init();
	dtex_async_load_multi_textures_init();
	dtex_async_load_c3_init();
	dtex_async_load_c2_init();
	dtex_async_load_c2_from_c3_init();

	dtex_render_init();

	dtex_texture_pool_init();

	LOADER = dtexloader_create();

	if (CFG.open_c4) {
		C4 = dtex_c4_create(CFG.c4_tex_size, 1);
	}
	if (CFG.open_c3) {
		C3 = dtex_c3_create(CFG.c3_tex_size, false);	
	}
 	if (CFG.open_c1) {
 		C1 = dtex_c1_create(CFG.c1_tex_size);		
 	}
 	if (CFG.open_c2) {
 		C2 = dtex_c2_create(CFG.c2_tex_size, true, 0, CFG.open_cg, CFG.src_extrude);
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
dtexf_load_pkg(const char* name, const char* epe_path, float scale, int lod) {
	return dtex_load_pkg(LOADER, name, epe_path, scale, lod);
}

void 
dtexf_unload_pkg(struct dtex_package* pkg) {
	dtex_unload_pkg(LOADER, pkg);
}

int 
dtexf_preload_all_textures(const char* path, struct dtex_package* pkg, float scale) {
	return dtex_preload_all_textures(path, LOADER, pkg, scale);
}

void 
dtexf_preload_texture(struct dtex_package* pkg, int idx, float scale) {
	dtex_preload_texture(LOADER, pkg, idx, scale);
}

void
dtexf_load_texture(struct dtex_package* pkg, int idx) {
	dtex_load_texture(LOADER, pkg, idx);
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
//	struct dtex_tp_pos* pos = dtex_c3_load_tex(C3, src_tex, &dst_tex);
//
//	dtexloader_unload_tex(src_tex);
//	free(src_tex);
//
//	return dtex_sprite_create(dst_tex, pos);
//}

/************************************************************************/
/* C4                                                                   */
/************************************************************************/

void
dtexf_c4_load(struct dtex_package* pkg) {
	if (C4) {
		dtex_c4_load(C4, pkg);
	}
}

void 
dtexf_c4_load_end(bool async) {
	if (C4) {
		dtex_c4_load_end(C4, LOADER, async);
	}
}

void 
dtexf_c4_clear() {
	if (C4) {
		dtex_c4_clear(C4);
	}
}

/************************************************************************/
/* C3                                                                   */
/************************************************************************/

void
dtexf_c3_load(struct dtex_package* pkg, float scale) {
	if (C3) {
		dtex_c3_load(C3, pkg, scale, false);
	}
}

void 
dtexf_c3_load_end(bool async) {
	if (C3) {
		dtex_c3_load_end(C3, LOADER, async);
	}
}

void 
dtexf_c3_clear() {
	if (C3) {
		dtex_c3_clear(C3);
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
		dtex_c2_load_spr(C2, pkg, spr_id);		
	}
}

void 
dtexf_c2_load_tex(int tex_id, int tex_width, int tex_height, int key) {
	if (C2) {
		dtex_c2_load_tex(C2, tex_id, tex_width, tex_height, key);
	}
}

void 
dtexf_c2_load_end() {
	if (C2) {
		dtex_c2_load_end(C2, LOADER);
	}
}

void 
dtexf_c2_remove_tex(int key) {
	if (C2) {
		dtex_c2_remove_tex(C2, key);
	}
}

void 
dtexf_c2_reload_begin() {
	if (C2) {
		dtex_c2_reload_begin(C2);
	}
}

void 
dtexf_c2_reload_tex(int tex_id, int tex_width, int tex_height, int key) {
	if (C2) {
		dtex_c2_reload_tex(C2, tex_id, tex_width, tex_height, key);
	}
}

void 
dtexf_c2_reload_end() {
	if (C2) {
		dtex_c2_reload_end(C2);
	}
}

float* 
dtexf_c2_lookup_texcoords(int pkg_id, int spr_id, int* dst_tex) {
	if (C2) {
		return dtex_c2_query_spr(C2, pkg_id, spr_id, dst_tex);
	} else {
		return NULL;
	}
}

float* 
dtexf_c2_query_tex(int key, int* out_texid) {
	if (C2) {
		return dtex_c2_query_tex(C2, key, out_texid);
	} else {
		return NULL;
	}
}

void 
dtexf_c2_clear_from_cg() {
	if (C2) {
		dtex_c2_clear_from_cg(C2, LOADER);
	}
}

void 
dtexf_c2_clear() {
	if (C2) {
		dtex_c2_clear(C2);
	}
}

//void 
//dtexf_c2_lookup_node(struct ej_texture* ori_tex, float* ori_vb, 
//	struct dtex_texture** out_tex, struct dtex_tp_pos** out_pos) {
//
//	if (C2 == NULL) {
//		return;
//	}
//
//	struct dtex_rect rect;
//	_get_pic_ori_rect(1/ori_tex->width, 1/ori_tex->height, ori_vb, &rect);
//
//	dtexc2_query_map_addr(C2, ori_tex->id, &rect, out_tex, out_pos);
//}

/************************************************************************/
/* C1                                                                   */
/************************************************************************/

void 
dtexf_c1_clear() {
	if (C1) {
		dtex_c1_clear(C1);
	}
}

void 
dtexf_c1_bind() {
	if (C1) {
		dtex_c1_bind(C1);
	}
}

void 
dtexf_c1_unbind() {
	if (C1) {
		dtex_c1_unbind(C1);
	}
}

uint32_t 
dtexf_c1_get_texture_id() {
	if (C1) {
		return dtex_c1_get_texture_id(C1);
	} else {
		return 0;
	}
}

uint32_t 
dtexf_c1_get_texture_size() {
	if (C1) {
		return dtex_c1_get_texture_size(C1);
	} else {
		return 0;
	}
}

//void 
//dtexf_c1_update(struct dtex_package* pkg, struct ej_sprite* spr) {
//	dtex_c1_update(C1, C2, pkg, spr);
//}

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
/* CG                                                                   */
/************************************************************************/

struct dtex_cg* 
dtexf_get_cg() {
	return C2 ? dtex_c2_get_cg(C2) : NULL;
}

/************************************************************************/
/* CS                                                                   */
/************************************************************************/

void 
dtexf_cs_create() {
	CS = dtex_cs_create();
}

void 
dtexf_cs_on_size(int width, int height) {
	if (CS) {
		dtex_cs_on_size(CS, width, height);
	}
}

void 
dtexf_cs_bind() {
	if (CS) {
		dtex_cs_bind(CS);
	}
}

void 
dtexf_cs_unbind() {
	if (CS) {
		dtex_cs_unbind(CS);
	}
}

void 
dtexf_cs_draw_to_screen(void (*before_draw)(void* ud), void* ud) {
	if (CS) {
		dtex_cs_draw_to_screen(CS, before_draw, ud);
	}
}

/************************************************************************/
/* async load texture                                                   */
/************************************************************************/

void 
dtexf_async_load_texture(struct dtex_package* pkg, int idx) {
	dtex_async_load_one_texture(pkg, idx, "normal");
}

bool 
dtexf_async_load_texture_with_c3(struct dtex_package* pkg, int* spr_ids, int spr_count) {
	return dtex_async_load_c3(LOADER, C3, pkg, spr_ids, spr_count);
}

bool 
dtexf_async_load_texture_with_c2(struct dtex_package* pkg, int* spr_ids, int spr_count) {
	return dtex_async_load_c2(LOADER, C2, pkg, spr_ids, spr_count);
}

bool 
dtexf_async_load_texture_with_c2_from_c3(struct dtex_package* pkg, int* spr_ids, int spr_count) {
	return dtex_async_load_c2_from_c3(LOADER, C2, C3, pkg, spr_ids, spr_count);
}

/************************************************************************/
/* update for async loading                                             */
/************************************************************************/

void 
dtexf_update() {
	dtex_async_loader_update();
	dtex_c2_strategy_update();
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
//	struct dtex_tp_pos* src_pos, struct draw_params* params, const int32_t part_screen[8]) {
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
  	} 
	if (C2) {
		dtex_c2_debug_draw(C2);
	}
	if (C3) {
		dtex_c3_debug_draw(C3);
	}
	if (C4) {
		dtex_c4_debug_draw(C4);
	}
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
//	tex = dtex_gl_create_texture(DTEX_TF_RGBA8, width, height, buf_uncompressed, 0, 0);
//#endifø
//	free(buf_uncompressed);
//
//// 	struct dtex_texture src_tex;
//// 	src_tex.id = tex;
//// 	src_tex.width = width;
//// 	src_tex.height = height;
//// 	src_tex.type = DTEX_TT_RAW;
//// 	src_tex.t.RAW.format = TEXTURE8;
//// 	src_tex.t.RAW.id_alpha = 0;
//// 
//// 	struct dtex_texture* dst_tex = NULL;
//// 	dtex_c3_load_tex(C3, &src_tex, &dst_tex);
//}

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
