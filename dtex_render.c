#include "dtex_render.h"
#include "dtex_gl.h"
#include "dtex_shader.h"
#include "dtex_target.h"
#include "dtex_texture.h"
#include "dtex_res_cache.h"
#include "dtex_screen.h"
#include "dtex_typedef.h"

#include <assert.h>
#include <string.h>

struct render_state {
	struct dtex_texture* dst;
	struct dtex_target* target;
//	float scr_w, scr_h;
	int ori_target;
};

static struct render_state RS;

void 
dtex_render_init() {
	memset(&RS, 0, sizeof(RS));
}

static inline void 
_before_all_draw() {
#ifndef USED_IN_EDITOR
	// dtex_gl_bind_vertex_array(1);
	dtex_shader_texture(0);
	dtex_shader_program(PROGRAM_NULL);
#endif // USED_IN_EDITOR

	assert(RS.target == NULL);
	RS.target = dtex_res_cache_fetch_target();

	RS.ori_target = dtex_target_bind(RS.target);
}

static inline void 
_after_all_draw() {
	assert((RS.dst && RS.target) || (!RS.dst && !RS.target));
	if (!RS.dst) {
		return;
	}

	dtex_shader_flush();

	dtex_target_unbind_texture(RS.target);
	dtex_target_unbind(RS.ori_target);  
	dtex_res_cache_return_target(RS.target);

	RS.dst = NULL;
	RS.target = NULL;

	float scr_w, scr_h, scr_s;
	dtex_get_screen(&scr_w, &scr_h, &scr_s);
	dtex_gl_viewport(0, 0, scr_w, scr_h);

#ifndef USED_IN_EDITOR
	ej_shader_texture(0, 0);
	ej_shader_program(PROGRAM_DEFAULT, NULL);
	// dtex_gl_bind_vertex_array(1);
	// ej_shader_reset();
#endif // USED_IN_EDITOR
}

static inline void
_before_draw(struct dtex_texture* tex) {
	// 	if (format == KTX) {
	// 		shader_program(PROGRAM_SPRITE_KTX);
	// 	} else if (format == PKMC) {
	// 		shader_program(PROGRAM_SPRITE_PKM);
	// 	} else {
	// 		shader_program(PROGRAM_SPRITE);
	// 	} 

	if (tex->type == DTEX_TT_RAW && tex->t.RAW.format == PKMC) {
		dtex_shader_program(PROGRAM_ETC1);
	} else {
		dtex_shader_program(PROGRAM_NORMAL);
	}
}

static inline void 
_before_target_draw(struct dtex_texture* src, struct dtex_texture* dst) {
	assert(RS.dst == NULL);
	RS.dst = dst;

	dtex_target_bind_texture(RS.target, dst->id);

	dtex_gl_viewport(0, 0, dst->width, dst->height);

	_before_draw(src);
}

static inline void
_draw(const float vb[16], struct dtex_texture* src) {
	if (src->type == DTEX_TT_RAW && src->t.RAW.format == PKMC) {
		//		shader_draw_separate(vb, src->id, src->id_alpha);
	} else { 
		assert(src->id != 0);
		dtex_shader_texture(src->id);
		dtex_shader_draw(vb);
	}
}

void 
dtex_draw_to_texture(struct dtex_texture* src, struct dtex_texture* dst, const float vb[16]) {
	if (RS.dst == NULL) {
		_before_all_draw();
		_before_target_draw(src, dst);
	} else if (RS.dst != dst) {
		_before_target_draw(src, dst);
	}

	_draw(vb, src);
}

void 
dtex_draw_finish() {
	_after_all_draw();
}