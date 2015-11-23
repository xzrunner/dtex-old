#include "dtex_target.h"
#include "dtex_file.h"
#include "dtex_log.h"
#include "dtex_shader.h"

#include "ejoy2d.h"

#include <opengl.h>

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

struct dtex_target {
	GLuint target_id;
	GLuint texture_id;
};

struct dtex_target* 
dtex_target_create() {
    dtex_info("dtex_target: new target");
	struct dtex_target* target = (struct dtex_target*)malloc(sizeof(struct dtex_target));
	glGenFramebuffers(1, &target->target_id);
	target->texture_id = 0;
	return target;
}

void 
dtex_target_release(struct dtex_target* target) {
	glDeleteFramebuffers(1, &target->target_id);
	free(target);
}

static inline int
_check_framebuffer_status() {
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	switch(status)
	{
	case GL_FRAMEBUFFER_COMPLETE:
		return 1;
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		dtex_warning("Framebuffer incomplete: Attachment is NOT complete.\n");
		return 0;
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		dtex_warning("Framebuffer incomplete: No image is attached to FBO.\n");
		return 0;
#if !defined(_WIN32) && !defined(__MACOSX)
	case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
		dtex_warning("Framebuffer incomplete: Attached images have different dimensions.\n");
		return 0;
#endif
	case GL_FRAMEBUFFER_UNSUPPORTED:
		dtex_warning("Unsupported by FBO implementation.\n");
		return 0;
	default:
		dtex_warning("Unknow error.\n");
		return 0;
	}
}

void 
dtex_target_bind_texture(struct dtex_target* target, int texid) {
	texid = dtex_gl_texture_id(texid);

	if (target->texture_id == texid) {
		return;
	}
	assert(dtex_shader_get_target() == target->target_id);

//	int ori = dtex_target_bind(target);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texid, 0);
	int status = _check_framebuffer_status();
	assert(status);
	target->texture_id = texid;

//	dtex_target_unbind(ori);
}

void 
dtex_target_unbind_texture(struct dtex_target* target) {
	target->texture_id = 0;
}

int 
dtex_target_bind(struct dtex_target* target) {
	int ori = dtex_shader_get_target();
	dtex_shader_set_target(target->target_id);
	glBindFramebuffer(GL_FRAMEBUFFER, target->target_id);
	return ori;
}

void 
dtex_target_unbind(int ori_target) {
	dtex_shader_set_target(ori_target);
	glBindFramebuffer(GL_FRAMEBUFFER, ori_target);
}