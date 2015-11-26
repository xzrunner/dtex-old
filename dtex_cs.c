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

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

struct dtex_cs {
	struct dtex_texture* texture;
	struct dtex_target* target;

	int last_target;
};

struct dtex_cs* 
dtex_cs_create() {
	int sz = sizeof(struct dtex_cs);
	struct dtex_cs* cs = (struct dtex_cs*)malloc(sz);
	cs->texture = NULL;
	cs->target = dtex_res_cache_fetch_target();
	return cs;
}

void 
dtex_cs_on_size(struct dtex_cs* cs, int width, int height) {
	if (cs->texture) {
		dtex_texture_release(cs->texture);
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

	int last_target = dtex_target_bind(cs->target);
	dtex_target_bind_texture(cs->target, cs->texture->id);
	dtex_target_unbind(last_target);
}

void 
dtex_cs_release(struct dtex_cs* cs) {
	free(cs);
}

void 
dtex_cs_bind(struct dtex_cs* cs) {
	cs->last_target = dtex_target_bind(cs->target);
	dtex_target_bind_texture(cs->target, cs->texture->id);

	dtex_shader_begin();

	dtex_gl_viewport(0, 0, cs->texture->width, cs->texture->height);
}

void dtex_cs_unbind(struct dtex_cs* cs) {
	dtex_shader_end();

	dtex_target_unbind(cs->last_target);

	float scr_w, scr_h, scr_s;
	dtex_get_screen(&scr_w, &scr_h, &scr_s);
	dtex_gl_viewport(0, 0, scr_w, scr_h);
}

void 
dtex_cs_draw_to_screen(struct dtex_cs* cs) {
	float vb[16];
	vb[0] = -1; vb[1] = -1; vb[2] = 0; vb[3] = 0;
	vb[4] =  1; vb[5] = -1; vb[6] = 1; vb[7] = 0;
	vb[8] =  1; vb[9] =  1; vb[10]= 1; vb[11]= 1;
	vb[12]= -1; vb[13]=  1; vb[14]= 0; vb[15]= 1;

	dtex_shader_begin();

	dtex_shader_set_texture(cs->texture->id);
	dtex_shader_draw(vb);

	dtex_shader_end();
}

void 
dtex_cs_debug_draw(struct dtex_cs* cs) {
	dtex_debug_draw(cs->texture->id, 2);
}