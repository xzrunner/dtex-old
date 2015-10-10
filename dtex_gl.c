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

#ifdef USE_EJ_RENDER
static struct ej_render* EJ_R = NULL;
#endif // USE_EJ_RENDER

void 
dtex_gl_init(struct ej_render* R) {
	EJ_R = R;
}

#ifdef USE_EJ_RENDER

void
dtex_gl_create_texture(int type, int width, int height, const void* data, int channel, int* gl_id, int* uid_3rd) {
	if (type == DTEX_TF_INVALID) {
		return;
	}

	*uid_3rd = ej_render_texture_create(EJ_R, width, height, EJ_TEXTURE_RGBA8, TEXTURE_2D, 0);
	ej_render_texture_update(EJ_R, *uid_3rd, width, height, data, 0, 0);

	*gl_id = render_get_texture_gl_id(EJ_R, *uid_3rd);
	dtex_stat_add_texture(*gl_id, width, height);
}

#else

unsigned int
_create_texture(int channel) {
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
dtex_gl_create_texture(int type, int width, int height, const void* data, int channel) {
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

	unsigned int id = _create_texture(channel);
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

#endif // USE_EJ_RENDER

void 
dtex_gl_release_texture(unsigned int id, int channel) {
	glActiveTexture(GL_TEXTURE0 + channel);
	glDeleteTextures(1, &id);

	dtex_shader_texture(0);

	dtex_stat_delete_texture(id);
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

int 
dtex_gl_get_max_texture_size() {
	int max_size;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_size);
	return max_size;
}

bool 
dtex_gl_out_of_memory() {
	GLenum err = glGetError();
	return err == GL_OUT_OF_MEMORY;
}

bool
dtex_gl_istexture(unsigned int id) {
	return glIsTexture(id);
}