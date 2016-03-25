#include "dtex_c1.h"
#include "dtex_target.h"
#include "dtex_texture.h"
#include "dtex_res_cache.h"
#include "dtex_debug.h"
#include "dtex_gl.h"

#include <stdlib.h>
#include <string.h>

struct dtex_c1 {
	struct dtex_texture* texture;
	struct dtex_target* target;

	int last_target;
};

struct dtex_c1* 
dtex_c1_create(int texture_size) {
	struct dtex_c1* c1 = (struct dtex_c1*)malloc(sizeof(struct dtex_c1));
	memset(c1, 0, sizeof(struct dtex_c1));

	c1->texture = dtex_res_cache_fetch_mid_texture(texture_size);
	c1->target = dtex_res_cache_fetch_target();
	c1->last_target = 0;

	return c1;
}

void 
dtex_c1_release(struct dtex_c1* c1) {
	free(c1);
}

void 
dtex_c1_clear(struct dtex_c1* c1, float xmin, float ymin, float xmax, float ymax) {
	dtex_gl_clear_color2(xmin, ymin, xmax, ymax);
}

void 
dtex_c1_bind(struct dtex_c1* c1) {
	c1->last_target = dtex_target_bind(c1->target);
	dtex_target_bind_texture(c1->target, c1->texture->id);
}

void 
dtex_c1_unbind(struct dtex_c1* c1) {
	dtex_target_unbind_texture(c1->target);
	dtex_target_unbind(c1->last_target);
}

//void 
//dtex_c1_update(struct dtex_c1* c1, struct dtex_c2* c2, struct dtex_package* pkg, struct ej_sprite* spr) {
//	int ori = dtex_target_bind(c1->target);
//
////	float w, h, s;
////	dtex_get_screen(&w, &h, &s);
////	glViewport(0, 0, c1->texture->width, c1->texture->height);
//
//	dtex_gl_clear_color(0, 0, 0, 1);
//
//	dtex_shader_program(DTEX_PROGRAM_NORMAL);
//	dtex_ej_sprite_draw(pkg, c2, spr, NULL);
//
////	dtex_shader_flush();
////	glViewport(0, 0, w, h);
//
//	dtex_target_unbind(ori);
//}

uint32_t 
dtex_c1_get_texture_id(struct dtex_c1* c1) {
	return c1->texture->id;
}

uint32_t 
dtex_c1_get_texture_size(struct dtex_c1* c1) {
	return c1->texture->width;
}

void 
dtex_c1_debug_draw(struct dtex_c1* c1) {
	dtex_debug_draw(c1->texture->id, 3);
}