#include "dtex_target.h"
#include "dtex_shader.h"
#include "dtex_gl.h"

#include <fs_file.h>
#include <opengl.h>
#include <logger.h>

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

struct dtex_target {
	GLuint target_id;
	GLuint texture_id;
};

#define MAX_LAYER 8
struct stack {
	int depth;
	int layers[MAX_LAYER];
};

static struct stack S;

void 
dtex_target_stack_init() {
	S.depth = 0;
	S.layers[S.depth++] = dtex_shader_get_target();
}

void 
dtex_target_stack_release() {
	S.depth = 0;
}

struct dtex_target* 
dtex_target_create() {
    LOGI("%s", "dtex_target: new target");
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
		LOGW("%s", "Framebuffer incomplete: Attachment is NOT complete.\n");
		return 0;
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		LOGW("%s", "Framebuffer incomplete: No image is attached to FBO.\n");
		return 0;
#if !defined(_WIN32) && !defined(__MACOSX)
	case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
		LOGW("%s", "Framebuffer incomplete: Attached images have different dimensions.\n");
		return 0;
#endif
	case GL_FRAMEBUFFER_UNSUPPORTED:
		LOGW("%s", "Unsupported by FBO implementation.\n");
		return 0;
	default:
		LOGW("%s", "Unknow error.\n");
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

//	dtex_target_bind(target);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texid, 0);
	int status = _check_framebuffer_status();
	assert(status);
	target->texture_id = texid;

//	dtex_target_unbind();
}

void 
dtex_target_unbind_texture(struct dtex_target* target) {
	target->texture_id = 0;
}

void 
dtex_target_bind(struct dtex_target* target) {	
	assert(S.depth < MAX_LAYER);
	dtex_shader_flush();

	int curr = S.layers[S.depth - 1];
	if (curr != target->target_id) {
		dtex_shader_set_target(target->target_id);
		glBindFramebuffer(GL_FRAMEBUFFER, target->target_id);
	}
	S.layers[S.depth++] = target->target_id;
}

void 
dtex_target_unbind() {
	assert(S.depth > 1);
	dtex_shader_flush();

	int curr = S.layers[S.depth - 1];
	--S.depth;
	int id = S.layers[S.depth - 1];
	if (curr != id) {
		dtex_shader_set_target(id);
		glBindFramebuffer(GL_FRAMEBUFFER, id);
	}
}