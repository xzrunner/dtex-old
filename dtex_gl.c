#include "dtex_gl.h"
#include "dtex_shader.h"
#include "dtex_statistics.h"
#include "dtex_typedef.h"
#include "dtex_log.h"
#include "dtex_pvr.h"
#include "dtex_etc1.h"
#include "dtex_math.h"

#include <opengl.h>
#include <stdlib.h>

#define COMPRESSED_RGBA_PVRTC_4BPPV1_IMG 4
#define COMPRESSED_RGBA_PVRTC_2BPPV1_IMG 2

#ifndef USED_IN_EDITOR
static struct ej_render* EJ_R = NULL;

void 
dtex_gl_init(struct ej_render* R) {
	EJ_R = R;
}

static inline void
_create_texture_ej(int type, int width, int height, const void* data, int channel, int* gl_id, int* uid_3rd) {
	if (type == DTEX_TF_INVALID) {
		return;
	}

	*uid_3rd = ej_render_texture_create(EJ_R, width, height, EJ_TEXTURE_RGBA8, TEXTURE_2D, 0);

	ej_render_texture_update(EJ_R, *uid_3rd, width, height, data, 0, 0);

	*gl_id = ej_render_get_texture_gl_id(EJ_R, *uid_3rd);
	dtex_stat_add_texture(*gl_id, width, height);
}

#endif // USED_IN_EDITOR

unsigned int
_gen_texture_dtex(int channel) {
	unsigned int id = 0;

	glActiveTexture(GL_TEXTURE0 + channel);
	glGenTextures(1, &id);

	dtex_shader_texture(id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	return id;
}

unsigned int 
_create_texture_dtex(int type, int width, int height, const void* data, int channel) {
	if (type == DTEX_TF_INVALID) {
		return 0;
	}

	// todo
	if ((type == DTEX_TF_RGBA8) || (IS_POT(width) && IS_POT(height))) {
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	} else {
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	}

	GLint _format;
	GLenum _type;

	bool is_compressed;
	unsigned int size = 0;
	uint8_t* uncompressed = NULL;

	switch (type) {
	case DTEX_TF_RGBA8:
		is_compressed = false;
		_format = GL_RGBA;
		_type = GL_UNSIGNED_BYTE;
		break;
	case DTEX_TF_RGBA4:
		is_compressed = false;
		_format = GL_RGBA;
		_type = GL_UNSIGNED_SHORT_4_4_4_4;
		break;
	case DTEX_TF_PVR2:
#ifdef __APPLE__
		is_compressed = true;
		_type = COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
		size = width * height * 8 * _type / 16;
#endif // __APPLE__
		break;
	case DTEX_TF_PVR4:
#ifdef __APPLE__
		is_compressed = true;
		_type = COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
		size = width * height * 8 * _type / 16;
#else
		is_compressed = false;
		_format = GL_RGBA;
		_type = GL_UNSIGNED_BYTE;
		uncompressed = dtex_pvr_decode(data, width, height);
#endif // __APPLE__
		break;
	case DTEX_TF_ETC1:
#ifdef __ANDROID__
		is_compressed = true;
		_type = GL_ETC1_RGB8_OES;
		size = width * height * 4 / 8;
#else
		is_compressed = false;
		_format = GL_RGBA;
		_type = GL_UNSIGNED_BYTE;
		uncompressed = dtex_etc1_decode(data, width, height);
#endif // __ANDROID__
		break;
	default:
		dtex_fault("dtex_gl_create_texture: unknown texture type.");
	}

	unsigned int id = _gen_texture_dtex(channel);
	if (is_compressed) {
		glCompressedTexImage2D(GL_TEXTURE_2D, 0, _type, width, height, 0, size, data);	
	} else {
		if (uncompressed) {
			glTexImage2D(GL_TEXTURE_2D, 0, _format, (GLsizei)width, (GLsizei)height, 0, _format, _type, uncompressed);
			free(uncompressed);
		} else {
			glTexImage2D(GL_TEXTURE_2D, 0, _format, (GLsizei)width, (GLsizei)height, 0, _format, _type, data);
		}
	}
	dtex_stat_add_texture(id, width, height);
	return id;
}

void
dtex_gl_create_texture(int type, int width, int height, const void* data, int channel, int* gl_id, int* uid_3rd, bool create_by_ej) {
	if (type == DTEX_TF_INVALID) {
		return;
	}

	if (create_by_ej) {
#ifndef USED_IN_EDITOR
		_create_texture_ej(type, width, height, data, channel, gl_id, uid_3rd);
#else
		*uid_3rd = 0;
		*gl_id = _create_texture_dtex(type, width, height, data, channel);
#endif // USED_IN_EDITOR
	} else {
		*uid_3rd = 0;
		*gl_id = _create_texture_dtex(type, width, height, data, channel);
	}
}

void 
dtex_gl_release_texture(unsigned int id, int channel) {
	glActiveTexture(GL_TEXTURE0 + channel);
	glDeleteTextures(1, &id);

	dtex_shader_texture(0);

	dtex_stat_delete_texture(id);
}

void 
dtex_release_ej_texture(int uid_3rd) {
	ej_render_release(EJ_R, EJ_TEXTURE, uid_3rd);
}

void 
dtex_gl_clear_color(float r, float g, float b, float a) {
	glClearColor(r, g, b, a);
	glClear(GL_COLOR_BUFFER_BIT);
}

void 
dtex_gl_viewport(int x, int y, int w, int h) {
	glViewport(x, y, w, h);
}

void 
dtex_gl_finish() {
	glFinish();
	glFlush();
}

void 
dtex_gl_bind_vertex_array(int id) {
	glBindVertexArray(id);
}

bool 
dtex_gl_out_of_memory() {
	GLenum err = glGetError();
	return err == GL_OUT_OF_MEMORY;
}

bool
dtex_gl_is_texture(unsigned int id) {
	return glIsTexture(id);
}

int 
dtex_gl_get_curr_texrute() {
	GLint texid;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &texid);
	return texid;
}

int 
dtex_gl_get_curr_target() {
	GLint fboid = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING , &fboid);
	return fboid;
}

void 
dtex_gl_check_error() {
	GLenum error = glGetError();
	if (error != GL_NO_ERROR
//		&& error != GL_INVALID_ENUM 
//		&& error != GL_INVALID_VALUE
//		&& error != GL_INVALID_OPERATION
//		&& error != GL_OUT_OF_MEMORY
//		&& error != GL_STACK_OVERFLOW 
//		&& error != GL_STACK_UNDERFLOW
	) {
		exit(1);
	}
}