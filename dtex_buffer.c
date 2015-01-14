#include "dtex_buffer.h"
#include "dtex_fbo.h"

#include "opengl.h"
#include "platform.h"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define MAX_TEX_COUNT 128
#define MAX_FBO_COUNT 8

struct dtex_buffer {
	GLuint tex_pool[MAX_TEX_COUNT];
	int next_tex, end_tex;
	unsigned int tex_edge;

	struct dtex_fbo* fbo_pool[MAX_FBO_COUNT];
	int fbo_size;
};

static inline int 
_get_max_texture_size() {
	int max;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max);
	return max;
}

static inline int 
_alloc_buffer(struct dtex_buffer* buf, int area_need) {
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glActiveTexture(GL_TEXTURE0);

	int edge = buf->tex_edge;
	uint8_t* empty_data = (uint8_t*)malloc(edge*edge*4);

	// for debug
	// static bool first = true;
	// if (first) {
	// 	memset(empty_data, 0xaa, edge*edge*4);		
	// 	first = false;
	// } else {
	// 	memset(empty_data, 0x00, edge*edge*4);		
	// }

	memset(empty_data, 0x00, edge*edge*4);

	int last_end = buf->end_tex;

	int expect_count = ceil((float)area_need / (edge*edge));
	int max_count = MIN(expect_count, MAX_TEX_COUNT);
    int end = buf->end_tex;
	for (int i = end; i < end + max_count; ++i) {
        pf_log("dtex_buffer: new texture %d\n", edge);
        
		GLuint tex = 0;
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)edge, (GLsizei)edge, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);		
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, (GLsizei)edge, (GLsizei)edge, GL_RGBA, GL_UNSIGNED_BYTE, &empty_data[0]);
		GLenum err = glGetError();
		if (err != GL_NO_ERROR) {
			// return 1 tex
			--buf->end_tex;
			buf->tex_pool[buf->end_tex] = 0;
			glDeleteTextures(1, &buf->tex_pool[buf->end_tex]);
			break;
		}

        buf->tex_pool[i] = tex;
        ++buf->end_tex;
	}

	free(empty_data);

	return edge * edge * (buf->end_tex - last_end);
}

struct dtex_buffer* 
dtexbuf_create() {
	struct dtex_buffer* buf = (struct dtex_buffer*)malloc(sizeof(*buf));
	memset(buf, 0, sizeof(*buf));
	buf->tex_edge = _get_max_texture_size();
	// buf->tex_edge = 256;
	return buf;	
}

int 
dtexbuf_reserve(struct dtex_buffer* buf, int area_need) {
	int cap = buf->tex_edge * buf->tex_edge * (buf->end_tex - buf->next_tex);
	if (cap >= area_need) {
		return area_need;
	} else {
		return _alloc_buffer(buf, area_need - cap);
	}
}

void 
dtexbuf_release(struct dtex_buffer* buf) {
	// glActiveTexture(GL_TEXTURE0);
	glDeleteTextures(buf->end_tex, &buf->tex_pool[0]);
	buf->next_tex = buf->end_tex = 0;

	for (int i = 0; i < buf->fbo_size; ++i) {
		dtex_del_fbo(buf->fbo_pool[i]);
	}
	buf->fbo_size = 0;

	free(buf);
}

unsigned int 
dtexbuf_fetch_texid(struct dtex_buffer* buf) {
	unsigned int tex_id = 0;
	if (buf->next_tex < buf->end_tex) {
		tex_id = buf->tex_pool[buf->next_tex];
		++buf->next_tex;
	}
	return tex_id;
}

bool 
dtexbuf_return_texid(struct dtex_buffer* buf, unsigned int texid) {
	if (buf->next_tex > 0) {
		buf->tex_pool[--buf->next_tex] = texid;
        return true;
	} else if (buf->end_tex < MAX_TEX_COUNT - 1) {
		buf->tex_pool[buf->end_tex++] = texid;
        return true;
	} else {
		return false;
	}
}

int 
dtexbuf_get_tex_edge(struct dtex_buffer* buf) {
	return buf->tex_edge;
}

struct dtex_fbo* 
dtexbuf_fetch_fbo(struct dtex_buffer* buf) {
	if (buf->fbo_size == 0) {
		return dtex_new_fbo();
	} else {
		struct dtex_fbo* fbo = buf->fbo_pool[buf->fbo_size - 1];
		--buf->fbo_size;
		return fbo;
	}
}

void 
dtexbuf_return_fbo(struct dtex_buffer* buf, struct dtex_fbo* fbo) {
	if (buf->fbo_size == MAX_FBO_COUNT) {
		dtex_del_fbo(fbo);
	} else {
		buf->fbo_pool[buf->fbo_size++] = fbo;
	}
}