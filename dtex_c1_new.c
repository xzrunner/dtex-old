#include "dtex_c1_new.h"
#include "dtex_buffer.h"
#include "dtex_texture.h"
#include "dtex_fbo.h"
#include "dtex_screen.h"
#include "dtex_shader.h"
#include "dtex_ej_sprite.h"
#include "dtex_draw.h"

#include <opengl.h>

#include <stdlib.h>
#include <string.h>

struct dtex_c1 {
	struct dtex_texture* texture;
	struct dtex_fbo* target;
};

struct dtex_c1* 
dtex_c1_create(struct dtex_buffer* buf) {
	struct dtex_c1* c1 = (struct dtex_c1*)malloc(sizeof(struct dtex_c1));
	memset(c1, 0, sizeof(struct dtex_c1));

	dtexbuf_reserve(buf, 1);
	c1->texture = dtex_new_tex(buf);

	c1->target = dtexbuf_fetch_fbo(buf);

	dtex_fbo_bind_texture(c1->target, c1->texture->tex);

	return c1;
}

void 
dtex_c1_release(struct dtex_c1* c1, struct dtex_buffer* buf) {
	dtex_del_tex(buf, c1->texture);
	dtexbuf_return_fbo(buf, c1->target);
	free(c1);
}

void 
dtex_c1_update(struct dtex_c1* c1, struct dtex_c2* c2, struct dtex_package* pkg, struct ej_sprite* spr) {
	dtex_fbo_bind(c1->target);

//	float w, h, s;
//	dtex_get_screen(&w, &h, &s);
//	glViewport(0, 0, c1->texture->width, c1->texture->height);

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	dtex_shader_program(PROGRAM_NORMAL);
	dtex_ej_sprite_draw(pkg, c2, spr, NULL);

	dtex_shader_flush();
//	glViewport(0, 0, w, h);

	dtex_fbo_unbind();
}

void 
dtex_c1_debug_draw(struct dtex_c1* c1) {
	dtex_debug_draw(c1->texture->tex);
}