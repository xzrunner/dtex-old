#include "dtex_texture.h"
#include "dtex_buffer.h"
#include "dtex_packer.h"
#include "dtex_fbo.h"

#include "opengl.h"
#include "platform.h"
#include "shader.h"

#include <assert.h>
#include <stdlib.h>

// todo how many nodes per new tex
#define MAX_TEX_SIZE 32

struct dtex_texture* 
dtex_new_tex(struct dtex_buffer* buf) {
	GLuint tex_id = dtexbuf_fetch_texid(buf);
	if (tex_id == 0) {
		return NULL;
	}

	struct dtex_texture* tex = (struct dtex_texture*)malloc(sizeof(struct dtex_texture));
	tex->tex = tex_id;
	tex->width = tex->height = dtexbuf_get_tex_edge(buf);
	tex->packer = NULL;

	return tex;
}

struct dtex_texture* 
dtex_new_tex_with_packer(struct dtex_buffer* buf, int packer_cap) {
	struct dtex_texture* tex = dtex_new_tex(buf);
	if (tex == NULL) {
		return NULL;
	}

	tex->packer = dtexpacker_create(tex->width, tex->height, packer_cap);

	return tex;
}

void 
dtex_del_tex(struct dtex_buffer* buf, struct dtex_texture* tex) {
	assert(tex);
	if (!dtexbuf_return_texid(buf, tex->tex)) {
		// glActiveTexture(GL_TEXTURE0);
		glDeleteTextures(1, &tex->tex); tex->tex = 0;		
	}

	if (tex->packer != NULL) {
		dtexpacker_release(tex->packer); 
		tex->packer = NULL;		
	}

	free(tex);
}

void 
dtex_clear_tex(struct dtex_texture* tex, struct dtex_buffer* buf) {
	assert(tex);

	struct dtex_fbo* fbo = dtexbuf_fetch_fbo(buf);
	dtex_fbo_bind_texture(fbo, tex);
	dtex_fbo_bind(fbo);

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	dtex_fbo_unbind();
	dtexbuf_return_fbo(buf, fbo);
}