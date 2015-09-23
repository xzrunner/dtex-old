#include "dtex_facade.h"
#include "dtex_loader_new.h"
#include "dtex_c3.h"
#include "dtex_c2.h"
#include "dtex_c1.h"
#include "dtex_buffer.h"
#include "dtex_async.h"
#include "dtex_utility.h"
#include "dtex_rrp.h"
#include "dtex_pts.h"
#include "dtex_draw.h"
#include "dtex_texture.h"
#include "dtex_pvr.h"
#include "dtex_etc1.h"
#include "dtex_packer.h"
#include "dtex_sprite.h"
#include "dtex_gl.h"
#include "dtex_file.h"
#include "dtex_log.h"
#include "dtex_texture_pool.h"

#include "sprite.h"

#include "cJSON.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define MAX_PACKAGE 512

struct dtex_loader* LOADER = NULL;
struct dtex_c3* C3 = NULL;
struct dtex_c2* C2 = NULL;
// struct dtex_c1* C1 = NULL;
struct dtex_buffer*	BUF = NULL;

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
	CFG.open_c1 = true;
	CFG.open_c2 = true;
	CFG.open_c3 = true;
	if (cfg) {
		_config(cfg);		
	}

	LOADER = dtexloader_create();

    if (CFG.open_c1 || CFG.open_c2 || CFG.open_c3) {
        BUF = dtexbuf_create();
    }
	if (CFG.open_c3) {
		C3 = dtexc3_create();	
	}
// 	if (CFG.open_c1) {
// 		C1 = dtexc1_create(BUF);		
// 	}
 	if (CFG.open_c2) {
 		C2 = dtexc2_create(BUF);		
 	}
}

void 
dtexf_release() {
 	if (C2) {
 		dtexc2_release(C2, BUF);		
 	}
// 	if (C1) {
// 		dtexc1_release(C1, BUF);		
// 	}
	if (C3) {
		dtexc3_release(C3, BUF);		
	}
	dtexbuf_release(BUF);
	if (C3) {
		dtexloader_release(LOADER);		
	}
}

struct dtex_package* 
dtexf_preload_pkg(const char* name, const char* path, int type) {
	return dtex_preload_pkg(LOADER, name, path, type);
}

//struct ej_sprite* 
//dtexf_create_sprite(const char* path) {
//	if (C3 == NULL) {
//		return NULL;
//	}
//
//	struct dtex_raw_tex* src_tex = dtexloader_load_image(path);
//
//	struct dtex_texture* dst_tex = NULL;
//	struct dp_pos* pos = dtexc3_load_tex(C3, src_tex, BUF, &dst_tex);
//
//	dtexloader_unload_tex(src_tex);
//	free(src_tex);
//
//	return dtex_sprite_create(dst_tex, pos);
//}

void
dtexf_c3_load_pkg(struct dtex_package* pkg, float scale) {
	if (C3) {
		dtexc3_preload_pkg(C3, pkg, scale);
	}
}

void 
dtexf_c3_load_pkg_finish() {
	if (C3 == NULL) {
		return;
	}

	dtexc3_preload_pkg_end(C3, LOADER, BUF);

	// relocate
	for (int i = 0; ; ++i) {
		struct dtex_package* pkg = dtex_get_pkg(LOADER, i);
		if (pkg == NULL) {
			break;
		} else {
			dtexc3_relocate(C3, pkg);
		}
	}
}

//void 
//dtexf_c2_load_begin() {
//	if (C2) {
//		dtexc2_preload_begin(C2);		
//	}
//}
//
//void 
//dtexf_c2_load_sprite(struct ej_package* pkg, const char* name) {
//	if (C2) {
//		int id = sprite_id(pkg, name);
//		dtexc2_preload_sprite(C2, pkg, id, -1);		
//	}
//}
//
//void 
//dtexf_c2_load_end() {
//	if (C2) {
//		dtexc2_preload_end(C2, BUF, LOADER, true);
//	}
//}
//
//static inline void
//_get_pic_ori_rect(int ori_w, int ori_h, float* ori_vb, struct dtex_rect* rect) {
//	float xmin = 1, ymin = 1, xmax = 0, ymax = 0;
//	for (int i = 0; i < 4; ++i) {
//		if (ori_vb[i*4+2] < xmin) xmin = ori_vb[i*4+2];
//		if (ori_vb[i*4+2] > xmax) xmax = ori_vb[i*4+2];
//		if (ori_vb[i*4+3] < ymin) ymin = ori_vb[i*4+3];
//		if (ori_vb[i*4+3] > ymax) ymax = ori_vb[i*4+3];
//	}
//	rect->xmin = ori_w * xmin;
//	rect->ymin = ori_h * ymin;
//	rect->xmax = ori_w * xmax;
//	rect->ymax = ori_h * ymax;
//}
//
//float* 
//dtexf_c2_lookup_texcoords(struct ej_texture* ori_tex, float* ori_vb, int* dst_tex) {
//	if (C2 == NULL) {
//		return NULL;
//	}
//
//	struct dtex_rect rect;
//	_get_pic_ori_rect(1/ori_tex->width, 1/ori_tex->height, ori_vb, &rect);
//
//	return dtexc2_lookup_texcoords(C2, ori_tex->id, &rect, dst_tex);
//}
//
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

//void 
//dtexf_c1_load_anim(struct ej_package* pkg, struct animation* ani, int action) {
//	if (C1) {
//		dtexc1_load_anim(C1, pkg, ani, action);		
//	}
//}

//bool 
//dtexf_c1_draw_anim(struct ej_package* pkg, struct animation* ani, int action, 
//	int frame, struct draw_params* params) {
//    if (C1 == NULL) {
//        return false;
//    }
//    frame /= 2;
//	return dtexc1_draw_anim(C1, pkg, ani, action, frame, params);
//}
//
//void 
//dtexf_async_load_spr(const char* pkg_name, const char* spr_name, const char* path) {
//	struct dtex_package* pkg = dtexc3_query_pkg(C3, pkg_name);
//
//	struct dtex_rect* rect[pkg->tex_size];
//	dtexc3_query_rect(C3, pkg_name, rect, pkg->tex_size);
//
//	int spr_id = sprite_id(pkg->ej_pkg, spr_name);
//	dtex_async_load_spr(LOADER, pkg->ej_pkg, rect, pkg->tex_size, spr_id, path);
//}

static inline void
_prepare_trans_pos(struct dtex_rect* rect, int tex_idx, struct dtex_raw_tex* dst_tex, struct dtex_img_pos* ori_pos, struct dtex_img_pos* dst_pos) {
	struct dtex_raw_tex* src_tex = dtex_pool_query(tex_idx);
	ori_pos->id = src_tex->id;
	ori_pos->id_alpha = src_tex->id_alpha;
	ori_pos->inv_width = src_tex->width;
	ori_pos->inv_height = src_tex->height;
	ori_pos->rect = *rect;

	dst_pos->id = dst_tex->id;
	dst_pos->id_alpha = dst_tex->id_alpha;
	dst_pos->inv_width = 1.0f / dst_tex->width;
	dst_pos->inv_height = 1.0f / dst_tex->height;
	dst_pos->rect.xmin = dst_pos->rect.ymin = 0;
	dst_pos->rect.xmax = dst_tex->width;
	dst_pos->rect.ymax = dst_tex->height;	
}

static inline void
_on_load_spr_task(struct ej_sprite_pack* ej_pkg, struct dtex_rect* rect, int spr_id, int tex_idx, struct dtex_raw_tex* dst_tex) {
	struct dtex_img_pos ori_pos, dst_pos;
	_prepare_trans_pos(rect, tex_idx, dst_tex, &ori_pos, &dst_pos);

	struct int_array* array = dtex_get_picture_id_set(ej_pkg, spr_id);

	dtex_relocate_spr(ej_pkg, array, tex_idx, &ori_pos, &dst_pos);
	dtexc2_preload_sprite(C2, ej_pkg, spr_id, tex_idx);
	dtex_relocate_spr(ej_pkg, array, tex_idx, &dst_pos, &ori_pos);

	free(array);
}

//static inline void
//_after_load_spr_task(struct ej_package* pkg, struct dtex_rect* rect, int spr_id, int tex_idx, struct dtex_raw_tex* dst_tex) {
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
//void 
//dtexf_update() {
//	_do_load_task();
//}
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
//	struct dtex_raw_tex src;
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

void 
dtexf_debug_draw() {
	if (/*!C1 && */!C2 && !C3) {
		return;
	}
//	dtexc1_debug_draw(C1);
	dtexc2_debug_draw(C2);
//	dtexc3_debug_draw(C3);
}

void 
dtexf_del_texture(int tex) {
	GLuint id = tex; 
	glDeleteTextures(1, &id);
}

void 
dtexf_test_pvr(const char* path) {
	uint32_t width, height;
	uint8_t* buf_compressed = dtex_pvr_read_file(path, &width, &height);
	assert(buf_compressed);

	uint8_t* buf_uncompressed = dtex_pvr_decode(buf_compressed, width, height);
	free(buf_compressed);

	GLuint tex = dtex_prepare_texture(GL_TEXTURE0);
#ifdef __APPLE__
	uint8_t* new_compressed = dtex_pvr_encode(buf_uncompressed, width, height);
    size_t sz = width * height / 2;
    glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG, width, height, 0, sz, new_compressed);
	free(new_compressed);
#else
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf_uncompressed);
#endif
	GLenum err = glGetError();
	if (err != GL_NO_ERROR){
		printf("Error uploading compressed texture level: 0. glError: 0x%04X", err);
	}
	free(buf_uncompressed);

	struct dtex_raw_tex src_tex;
	src_tex.format = TEXTURE8;
	src_tex.width = width;
	src_tex.height = height;
	src_tex.id = tex;
	src_tex.id_alpha = 0;

	struct dtex_texture* dst_tex = NULL;
	dtexc3_load_tex(C3, &src_tex, BUF, &dst_tex);
}

#ifndef __ANDROID__

void 
dtexf_test_etc1(const char* path) {
	uint32_t width, height;
	uint8_t* buf_compressed = dtex_etc1_read_file(path, &width, &height);
	assert(buf_compressed);

	uint8_t* buf_uncompressed = dtex_etc1_decode(buf_compressed, width, height);
	free(buf_compressed);

	GLuint tex = dtex_prepare_texture(GL_TEXTURE0);
#ifdef __ANDROID__
	// todo
#else
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf_uncompressed);
#endif
	GLenum err = glGetError();
	if (err != GL_NO_ERROR){
		printf("Error uploading compressed texture level: 0. glError: 0x%04X", err);
	}
	free(buf_uncompressed);

	struct dtex_raw_tex src_tex;
	src_tex.format = TEXTURE8;
	src_tex.width = width;
	src_tex.height = height;
	src_tex.id = tex;
	src_tex.id_alpha = 0;

	struct dtex_texture* dst_tex = NULL;
	dtexc3_load_tex(C3, &src_tex, BUF, &dst_tex);
}

#endif