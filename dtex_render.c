#include "dtex_render.h"
#include "dtex_gl.h"
#include "dtex_shader.h"
#include "dtex_target.h"
#include "dtex_texture.h"
#include "dtex_res_cache.h"
#include "dtex_screen.h"
#include "dtex_typedef.h"

#include <assert.h>

void 
dtex_render_before() {
#ifndef USED_IN_EDITOR
	dtex_gl_bind_vertex_array(0);
	dtex_shader_texture(0);
	dtex_shader_program(PROGRAM_NULL);
#endif // USED_IN_EDITOR
}

void 
dtex_render_after() {
#ifndef USED_IN_EDITOR
	ej_shader_texture(0, 0);
	ej_shader_program(PROGRAM_DEFAULT, NULL);
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
_before_target_draw(struct dtex_texture* src, struct dtex_texture* dst, 
struct dtex_target** target, float* scr_w, float* scr_h) {
	*target = dtex_res_cache_fetch_target();
	dtex_target_bind_texture(*target, dst->id);
	dtex_target_bind(*target);

	float s;
	dtex_get_screen(scr_w, scr_h, &s);
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

static inline void
_after_target_draw(struct dtex_target* target, float scr_w, float scr_h) {
	dtex_shader_flush();

	dtex_gl_viewport(0, 0, scr_w, scr_h);

	dtex_target_unbind();  
	dtex_target_unbind_texture(target);

	dtex_res_cache_return_target(target);
}


void 
dtex_draw_to_texture(struct dtex_texture* src, struct dtex_texture* dst, const float vb[16]) {
	struct dtex_target* target = NULL;
	float scr_w, scr_h;
	_before_target_draw(src, dst, &target, &scr_w, &scr_h);

	_draw(vb, src);

	_after_target_draw(target, scr_w, scr_h);
}