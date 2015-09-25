#include "dtex_fbo.h"
#include "dtex_texture.h"
#include "dtex_file.h"
#include "dtex_log.h"
#include "dtex_shader.h"

#include "opengl.h"

#include <ejoy2d.h>

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

struct dtex_fbo {
	GLuint target_id;
	GLuint texture_id;
};

struct dtex_fbo* 
dtex_new_fbo() {
    dtex_info("dtex_fbo: new fbo\n");
	struct dtex_fbo* fbo = (struct dtex_fbo*)malloc(sizeof(struct dtex_fbo));
	glGenFramebuffers(1, &fbo->target_id);
	fbo->texture_id = 0;
	return fbo;
}

void 
dtex_del_fbo(struct dtex_fbo* fbo) {
	glDeleteFramebuffers(1, &fbo->target_id);
	free(fbo);
}

static inline int
_check_framebuffer_status() {
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	switch(status)
	{
	case GL_FRAMEBUFFER_COMPLETE:
		dtex_info("++ fbo: Framebuffer complete.\n");
		return 1;
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		dtex_info("++ fbo: [ERROR] Framebuffer incomplete: Attachment is NOT complete.\n");
		return 0;
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		dtex_info("++ fbo: [ERROR] Framebuffer incomplete: No image is attached to FBO.\n");
		return 0;
#ifndef _WIN32
	case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
		dtex_info("++ fbo: [ERROR] Framebuffer incomplete: Attached images have different dimensions.\n");
		return 0;
#endif
	case GL_FRAMEBUFFER_UNSUPPORTED:
		dtex_info("++ fbo: [ERROR] Unsupported by FBO implementation.\n");
		return 0;
	default:
		dtex_info("++ fbo: [ERROR] Unknow error.\n");
		return 0;
	}
}

void 
dtex_shader_fbo(struct dtex_fbo* fbo) {
	dtex_shader_target(fbo->target_id);
}

void 
dtex_fbo_bind_texture(struct dtex_fbo* fbo, int texid) {
	if (fbo->texture_id == texid) {
		return;
	}

	dtex_fbo_bind(fbo);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texid, 0);
	int status = _check_framebuffer_status();
	assert(status);
	fbo->texture_id = texid;

	dtex_fbo_unbind();
}

void 
dtex_fbo_bind(struct dtex_fbo* fbo) {
	glBindFramebuffer(GL_FRAMEBUFFER, fbo->target_id);
}

void 
dtex_fbo_unbind() {
#ifdef _WIN32
	glBindFramebuffer(GL_FRAMEBUFFER, 1);
#else
	glBindFramebuffer(GL_FRAMEBUFFER, dtex_shader_get_target());
#endif
}