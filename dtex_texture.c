#include "dtex_texture.h"
#include "dtex_log.h"
#include "dtex_tp.h"
#include "dtex_gl.h"
#include "dtex_target.h"
#include "dtex_res_cache.h"
#include "dtex_hard_res.h"
#include "dtex_lod.h"
#include "dtex_shader.h"
#include "dtex_screen.h"

#include <opengl.h>

#include <string.h>
#include <stdlib.h>

#define MAX_TEXTURE 512

struct texture_pool {
	int count;
	struct dtex_texture textures[MAX_TEXTURE];
};

static struct texture_pool POOL;

static inline struct dtex_texture* 
_get_free_texture() {
	struct dtex_texture* tex = NULL;
	for (int i = 0; i < POOL.count; ++i) {
		if (POOL.textures[i].type == DTEX_TT_INVALID) {
			tex = &POOL.textures[i];
			tex->uid = i + QUAD_TEXID_IN_PKG_MAX;
			break;
		}
	}

	if (!tex && POOL.count < MAX_TEXTURE) {
		tex = &POOL.textures[POOL.count++];
		tex->uid = POOL.count - 1 + QUAD_TEXID_IN_PKG_MAX;
	}

	return tex;
}

struct dtex_texture* 
dtex_texture_create_raw(int lod) {
	struct dtex_texture* tex = _get_free_texture();
	if (!tex) {
		dtex_fault("dtex_create_raw_texture fail.");
		return NULL;
	}

	tex->type = DTEX_TT_RAW;
	tex->t.RAW.scale = 1;
	tex->t.RAW.lod_scale = dtex_lod_get_scale(lod);

	return tex;
}

struct dtex_texture* 
dtex_texture_create_mid(int edge) {
	struct dtex_texture* tex = _get_free_texture();
	if (!tex) {
		dtex_fault("dtex_texture_create_mid _get_free_texture fail.");
		return NULL;
	}

	if (edge > dtex_max_texture_size()) {
		edge = dtex_max_texture_size();
	}

	uint8_t* empty_data = (uint8_t*)malloc(edge*edge*4);
	if (!empty_data) {
		dtex_fault("dtex_texture_create_mid malloc fail.");
		return NULL;
	}
	memset(empty_data, 0x00, edge*edge*4);

	unsigned int gl_id;
	int uid_3rd;
	dtex_gl_create_texture(DTEX_TF_RGBA8, edge, edge, empty_data, 0, &gl_id, &uid_3rd, true);
	free(empty_data);
	if (dtex_gl_out_of_memory()) {
		dtex_fault("dtex_texture_create_mid dtex_gl_create_texture fail.");
		return NULL;
	}

	tex->type = DTEX_TT_MID;
	tex->id = gl_id;
	tex->uid_3rd = uid_3rd;
	tex->width = tex->height = edge;
	tex->inv_width = tex->inv_height = 1.0f / edge;
	tex->t.MID.tp = NULL;

	return tex;
}

void
dtex_texture_release(struct dtex_texture* tex) {
	if (!tex) { return; }

	if (tex->type == DTEX_TT_RAW) {
		if (tex->id != 0) {
			dtex_gl_release_texture(tex->id, 0);
		}
		if (tex->t.RAW.id_alpha != 0) {
			dtex_gl_release_texture(tex->t.RAW.id_alpha, 1);
		}
	} else if (tex->type == DTEX_TT_MID) {
		if (tex->id != 0) {
			dtex_gl_release_texture(tex->id, 0);
		}
		if (tex->t.MID.tp != NULL) {
			dtex_tp_release(tex->t.MID.tp); 
		}
	}

#ifndef USED_IN_EDITOR
	if (tex->uid_3rd != 0) {
		dtex_release_ej_texture(tex->uid_3rd);
	}
#endif // USED_IN_EDITOR

	memset(tex, 0, sizeof(*tex));
	tex->type = DTEX_TT_INVALID;
}

void 
dtex_texture_clear(struct dtex_texture* tex) {
	if (!tex) {
		return;
	}

	struct dtex_target* target = dtex_res_cache_fetch_target();

	int ori = dtex_target_bind(target);
	dtex_target_bind_texture(target, tex->id);

	dtex_gl_clear_color(0, 0, 0, 0);

	dtex_target_unbind_texture(target);
	dtex_target_unbind(ori);
	dtex_res_cache_return_target(target);
}
void 
dtex_texture_clear_part(struct dtex_texture* tex, float xmin, float ymin, float xmax, float ymax) {
	if (!tex) {
		return;
	}

	float scr_s, scr_w, scr_h;
	dtex_get_screen(&scr_w, &scr_h, &scr_s);
	dtex_gl_viewport(0, 0, tex->width, tex->height);

	dtex_gl_scissor(
		tex->width * xmin, 
		tex->height * ymin, 
		tex->width * (xmax - xmin), 
		tex->height * (ymax - ymin));

	struct dtex_target* target = dtex_res_cache_fetch_target();
	int ori = dtex_target_bind(target);
	dtex_target_bind_texture(target, tex->id);

	glEnable(GL_SCISSOR_TEST);
	dtex_gl_clear_color(0, 0, 0, 0);
	glDisable(GL_SCISSOR_TEST);

	dtex_target_unbind_texture(target);
	dtex_target_unbind(ori);

	dtex_res_cache_return_target(target);

	dtex_gl_viewport(0, 0, scr_w, scr_h);
}

//void 
//dtex_texture_clear_part(struct dtex_texture* tex, float xmin, float ymin, float xmax, float ymax) {
//	if (!tex) {
//		return;
//	}
//
//	float scr_s, scr_w, scr_h;
//	dtex_get_screen(&scr_w, &scr_h, &scr_s);
//	dtex_gl_viewport(0, 0, tex->width, tex->height);
//
//	struct dtex_target* target = dtex_res_cache_fetch_target();
//	dtex_target_bind_texture(target, tex->id);
//	dtex_target_bind(target);
//
//	dtex_shader_program(PROGRAM_SHAPE);
//	dtex_shader_texture(0);
//
//	// abgr
//	unsigned int color = 0x00000000;
//
//	float vb[3 * 6];
//	vb[0] = xmin;
//	vb[1] = ymin;
//	memcpy(&vb[2], &color, sizeof(color));
//
//	vb[3] = xmin;
//	vb[4] = ymax;
//	memcpy(&vb[5], &color, sizeof(color));
//
//	vb[6] = xmax;
//	vb[7] = ymax;
//	memcpy(&vb[8], &color, sizeof(color));
//
//	vb[9] = xmin;
//	vb[10] = ymin;
//	memcpy(&vb[11], &color, sizeof(color));
//
//	vb[12] = xmax;
//	vb[13] = ymin;
//	memcpy(&vb[14], &color, sizeof(color));
//
//	vb[15] = xmax;
//	vb[16] = ymax;
//	memcpy(&vb[17], &color, sizeof(color));
//
//	dtex_shader_draw_triangle(vb, 6);
//
//	dtex_shader_program(PROGRAM_NORMAL);
//
//	dtex_target_unbind();
//	dtex_target_unbind_texture(target);
//
//	dtex_res_cache_return_target(target);
//
//	dtex_gl_viewport(0, 0, scr_w, scr_h);
//}

void 
dtex_texture_pool_init() {
	memset(&POOL, 0, sizeof(POOL));
	for (int i = 0; i < MAX_TEXTURE; ++i) {
		POOL.textures[i].type = DTEX_TT_INVALID;
		POOL.textures[i].uid = i + QUAD_TEXID_IN_PKG_MAX;
	}
}

struct dtex_texture* 
dtex_texture_fetch(int uid) {
	int idx = uid - QUAD_TEXID_IN_PKG_MAX;
	if (idx < 0 || idx >= POOL.count) {
		return NULL;
	} else {
		return &POOL.textures[idx];
	}
}

unsigned int 
dtex_texture_get_gl_id(int uid) {
	struct dtex_texture* tex = dtex_texture_fetch(uid);
	if (tex) {
		return tex->id;
	} else {
		return 0;
	}
}
