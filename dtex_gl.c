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

#if !defined (VAO_DISABLE) && !defined (__ANDROID__)
// If your platform doesn't support VAO, comment it out.
// Or define VAO_DISABLE first
#define VAO_ENABLE


#if defined (GL_OES_vertex_array_object)
#define glBindVertexArray glBindVertexArrayOES
#define glGenVertexArrays glGenVertexArraysOES
#define glDeleteVertexArrays glDeleteVertexArraysOES
#endif

#endif

#define COMPRESSED_RGBA_PVRTC_4BPPV1_IMG 4
#define COMPRESSED_RGBA_PVRTC_2BPPV1_IMG 2

static int (*TEXTURE_CREATE)(int type, int width, int height, const void* data, int channel, unsigned int id);
static void (*TEXTURE_RELEASE)(int id);
static void (*TEXTURE_UPDATE)(const void* pixels, int x, int y, int w, int h, unsigned int id);
static  int (*TEXTURE_ID)(int id);

void 
dtex_gl_texture_init(int (*texture_create)(int type, int width, int height, const void* data, int channel, unsigned int id),
					 void (*texture_release)(int id),
					 void (*texture_update)(const void* pixels, int x, int y, int w, int h, unsigned int id),
					  int (*texture_id)(int id)) {
	TEXTURE_CREATE = texture_create;
	TEXTURE_RELEASE = texture_release;
	TEXTURE_UPDATE = texture_update;
	TEXTURE_ID = texture_id;
}

int 
dtex_gl_create_texture(int type, int width, int height, const void* data, int channel, unsigned int id) {
	id = TEXTURE_CREATE(type, width, height, data, channel, id);
	dtex_stat_add_texture(id, width, height);
	return id;
}

void 
dtex_gl_release_texture(int id) {
	dtex_shader_set_texture(0);
	TEXTURE_RELEASE(id);
	dtex_stat_delete_texture(id);
}

void 
dtex_gl_update_texture(const void* pixels, int x, int y, int w, int h, unsigned int id) {
	TEXTURE_UPDATE(pixels, x, y, w, h, id);
}

int 
dtex_gl_texture_id(int id) {
	return TEXTURE_ID(id);
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
dtex_gl_scissor(int x, int y, int w, int h) {
	glScissor(x, y, w, h);
}

void 
dtex_gl_finish() {
	glFinish();
	glFlush();
}

void 
dtex_gl_bind_vertex_array(int id) {
#ifdef VAO_ENABLE
	glBindVertexArray(id);
#endif
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
	if (error != GL_NO_ERROR) {
		exit(1);
	}
}