#include "dtex_cs.h"
#include "dtex_texture.h"
#include "dtex_log.h"
#include "dtex_gl.h"
#include "dtex_typedef.h"
#include "dtex_res_cache.h"
#include "dtex_target.h"
#include "dtex_shader.h"
#include "dtex_debug.h"

#include "dtex_screen.h"

#include <opengl.h>

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define MAX_RECT 512

struct dtex_cs {
	struct dtex_texture* texture;
	struct dtex_target* target;

	int last_target;

	struct dtex_cs_rect invalid_rect[MAX_RECT];
	int rect_count;

	float x, y, scale;
	bool dirty;
};

struct dtex_cs* 
dtex_cs_create() {
	int sz = sizeof(struct dtex_cs);
	struct dtex_cs* cs = (struct dtex_cs*)malloc(sz);

	cs->texture = NULL;
	cs->target = dtex_res_cache_fetch_target();
	cs->last_target = 0;

	cs->rect_count = 0;

	cs->x = cs->y = 0;
	cs->scale = 1;

	cs->dirty = false;

	return cs;
}

void 
dtex_cs_release(struct dtex_cs* cs) {
	free(cs);
}

void 
dtex_cs_on_size(struct dtex_cs* cs, int width, int height) {
	if (width <= 0 || height <= 0) {
		return;
	}

	if (cs->texture) {
		if (cs->texture->width == width && cs->texture->height == height) {
			return;
		} else {
			dtex_texture_release(cs->texture);
			cs->texture = NULL;
		}
	}

	uint8_t* empty_data = (uint8_t*)malloc(width*height*4);
	if (!empty_data) {
		dtex_fault("dtex_cs_on_size malloc fail.");
		return;
	}

	memset(empty_data, 0xff, width*height*4);

	int id = dtex_gl_create_texture(DTEX_TF_RGBA8, width, height, empty_data, 0, 0);
	free(empty_data);
	if (dtex_gl_out_of_memory()) {
		dtex_fault("dtex_cs_on_size dtex_gl_create_texture fail.");
		return;
	}

	struct dtex_texture* tex = dtex_texture_create_raw(0);
	tex->t.RAW.format = TEXTURE8;
	tex->width = width;
	tex->height = height;
	tex->inv_width = 1.0f / width;
	tex->inv_height = 1.0f / height;
	tex->t.RAW.scale = 1;
	tex->id = id;
	tex->t.RAW.id_alpha = 0;

	cs->texture = tex;
}

void 
dtex_cs_bind(struct dtex_cs* cs) {
	cs->last_target = dtex_target_bind(cs->target);
	dtex_target_bind_texture(cs->target, cs->texture->id);

//	dtex_shader_begin();
}

void 
dtex_cs_unbind(struct dtex_cs* cs) {
//	dtex_shader_end();

	dtex_target_unbind_texture(cs->target);
	dtex_target_unbind(cs->last_target);
}

void 
dtex_cs_add_inv_rect(struct dtex_cs* cs, struct dtex_cs_rect* rect) {
	if (cs->rect_count >= MAX_RECT) {
		// todo invalid all
		return;
	}

	cs->invalid_rect[cs->rect_count++] = *rect;
}

void 
dtex_cs_clear_inv_rects(struct dtex_cs* cs, float cam_x, float cam_y, float cam_scale) {
	float scr_w, scr_h, scr_s;
	dtex_get_screen(&scr_w, &scr_h, &scr_s);

	glEnable(GL_SCISSOR_TEST);

	for (int i = 0; i < cs->rect_count; ++i) {
		struct dtex_cs_rect* r = &cs->invalid_rect[i];
		
		float x = (r->xmin - cam_x) / cam_scale + scr_w * 0.5f,
			  y = (r->ymin - cam_y) / cam_scale + scr_h * 0.5f,
			  w = (r->xmax - r->xmin) / cam_scale,
			  h = (r->ymax - r->ymin) / cam_scale;
		dtex_gl_scissor(x, y, w, h);

		dtex_gl_clear_color(0, 1, 0, 1);
	}

	glDisable(GL_SCISSOR_TEST);
}

void 
dtex_cs_set_pos(struct dtex_cs* cs, float x, float y, float scale) {
	float w, h, s;
	dtex_get_screen(&w, &h, &s);
	float cx = 2 * x / w,
		  cy = 2 * y / h;
	if (cs->x == cx && cs->y == cy && cs->scale == scale) {
		return;
	}

	cs->x = cx;
	cs->y = cy;
	cs->scale = scale;

	cs->dirty = true;
}

bool 
dtex_cs_dirty(struct dtex_cs* cs) {
	return cs->dirty;
}

void 
dtex_cs_draw_to_screen(struct dtex_cs* cs) {
	float vb[16];

	float vx_min = (-1 + cs->x) * cs->scale,
		  vx_max = ( 1 + cs->x) * cs->scale,
		  vy_min = (-1 + cs->y) * cs->scale,
		  vy_max = ( 1 + cs->y) * cs->scale;
	vb[0] = vx_min; vb[1] = vy_min; vb[2] = 0; vb[3] = 0;
	vb[4] = vx_max; vb[5] = vy_min; vb[6] = 1; vb[7] = 0;
	vb[8] = vx_max; vb[9] = vy_max; vb[10]= 1; vb[11]= 1;
	vb[12]= vx_min; vb[13]= vy_max; vb[14]= 0; vb[15]= 1;

	dtex_shader_begin();

	dtex_shader_program(DTEX_PROGRAM_NORMAL);
	dtex_shader_set_texture(cs->texture->id);
	dtex_shader_draw(vb);

	dtex_shader_end();

	cs->rect_count = 0;
	cs->dirty = false;
}

void 
dtex_cs_reload(struct dtex_cs* cs) {
	if (cs->texture) {
		dtex_texture_reload(cs->texture);
	}
}

void 
dtex_cs_debug_draw(struct dtex_cs* cs) {
	if (cs->texture) {
		dtex_debug_draw(cs->texture->id, 2);
	}
}