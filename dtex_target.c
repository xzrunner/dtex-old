#include "dtex_target.h"
#include "dtex_texture.h"
#include "dtex_file.h"
#include "dtex_log.h"
#include "dtex_shader.h"

#include <opengl.h>

#include <ejoy2d.h>

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

struct dtex_target {
	GLuint target_id;
	GLuint texture_id;
};

struct dtex_target* 
dtex_new_target() {
    dtex_info("dtex_target: new target\n");
	struct dtex_target* target = (struct dtex_target*)malloc(sizeof(struct dtex_target));
	glGenFramebuffers(1, &target->target_id);
	target->texture_id = 0;
	return target;
}

void 
dtex_del_target(struct dtex_target* target) {
	glDeleteFramebuffers(1, &target->target_id);
	free(target);
}

static inline int
_check_framebuffer_status() {
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	switch(status)
	{
	case GL_FRAMEBUFFER_COMPLETE:
		dtex_info("++ target: Framebuffer complete.\n");
		return 1;
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		dtex_info("++ target: [ERROR] Framebuffer incomplete: Attachment is NOT complete.\n");
		return 0;
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		dtex_info("++ target: [ERROR] Framebuffer incomplete: No image is attached to FBO.\n");
		return 0;
#ifndef _WIN32
	case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
		dtex_info("++ target: [ERROR] Framebuffer incomplete: Attached images have different dimensions.\n");
		return 0;
#endif
	case GL_FRAMEBUFFER_UNSUPPORTED:
		dtex_info("++ target: [ERROR] Unsupported by FBO implementation.\n");
		return 0;
	default:
		dtex_info("++ target: [ERROR] Unknow error.\n");
		return 0;
	}
}

void 
dtex_target_bind_texture(struct dtex_target* target, int texid) {
	if (target->texture_id == texid) {
		return;
	}

	dtex_target_bind(target);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texid, 0);
	int status = _check_framebuffer_status();
	assert(status);
	target->texture_id = texid;

	dtex_target_unbind();
}

void 
dtex_target_unbind_texture(struct dtex_target* target) {
	target->texture_id = 0;
}

void 
dtex_target_bind(struct dtex_target* target) {
	glBindFramebuffer(GL_FRAMEBUFFER, target->target_id);
}

void 
dtex_target_unbind() {
	glBindFramebuffer(GL_FRAMEBUFFER, dtex_shader_get_target());
}