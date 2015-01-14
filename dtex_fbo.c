#include "dtex_fbo.h"
#include "dtex_texture.h"

#include "opengl.h"
#include "shader.h"
#include "platform.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

struct dtex_fbo {
	GLuint id;
	struct dtex_texture* tex;
};

struct dtex_fbo* 
dtex_new_fbo() {
    pf_log("dtex_fbo: new fbo\n");
	struct dtex_fbo* fbo = (struct dtex_fbo*)malloc(sizeof(struct dtex_fbo));
	glGenFramebuffers(1, &fbo->id);
	fbo->tex = NULL;
	return fbo;
}

void 
dtex_del_fbo(struct dtex_fbo* fbo) {
	glDeleteFramebuffers(1, &fbo->id);
	free(fbo);
}

static inline int
_check_framebuffer_status() {
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	switch(status)
	{
	case GL_FRAMEBUFFER_COMPLETE:
		pf_log("++ fbo: Framebuffer complete.\n");
		return 1;
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		pf_log("++ fbo: [ERROR] Framebuffer incomplete: Attachment is NOT complete.\n");
		return 0;
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		pf_log("++ fbo: [ERROR] Framebuffer incomplete: No image is attached to FBO.\n");
		return 0;
#ifndef _WIN32
	case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
		pf_log("++ fbo: [ERROR] Framebuffer incomplete: Attached images have different dimensions.\n");
		return 0;
#endif
	case GL_FRAMEBUFFER_UNSUPPORTED:
		pf_log("++ fbo: [ERROR] Unsupported by FBO implementation.\n");
		return 0;
	default:
		pf_log("++ fbo: [ERROR] Unknow error.\n");
		return 0;
	}
}

void 
dtex_shader_fbo(struct dtex_fbo* fbo) {
	shader_fbo(fbo->id);
}

void 
dtex_fbo_bind_texture(struct dtex_fbo* fbo, struct dtex_texture* tex) {
	if (fbo->tex == tex) {
		return;
	}

	dtex_fbo_bind(fbo);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex->tex, 0);
	int status = _check_framebuffer_status();
	assert(status);
	fbo->tex = tex;

	dtex_fbo_unbind();
}

void 
dtex_fbo_bind(struct dtex_fbo* fbo) {
	glBindFramebuffer(GL_FRAMEBUFFER, fbo->id);
}

void 
dtex_fbo_unbind() {
	glBindFramebuffer(GL_FRAMEBUFFER, shader_get_fbo());
}