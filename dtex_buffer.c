#include "dtex_buffer.h"
#include "dtex_target.h"
#include "dtex_gl.h"
#include "dtex_file.h"
#include "dtex_math.h"
#include "dtex_log.h"
#include "dtex_statistics.h"
#include "dtex_typedef.h"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define MAX_TEX_COUNT 128
#define MAX_FBO_COUNT 8

#define MAX_TEXTURE_SIZE 4096
//#define MAX_TEXTURE_SIZE 2048

struct tex_id {
	unsigned int gl_id;
	int uid_3rd;
};

struct dtex_buffer {
	struct tex_id tex_pool[MAX_TEX_COUNT];
	int next_tex, end_tex;
	unsigned int tex_edge;

	struct dtex_target* target_pool[MAX_FBO_COUNT];
	int target_size;
};

static inline int 
_get_max_texture_size() {
	return MIN(dtex_gl_get_max_texture_size(), MAX_TEXTURE_SIZE);
}

static inline int 
_alloc_buffer(struct dtex_buffer* buf, int area_need) {
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
        dtex_info("dtex_buffer: new texture %d\n", edge);
        
		int gl_id, uid_3rd;
		dtex_gl_create_texture(DTEX_TF_RGBA8, edge, edge, empty_data, 0, &gl_id, &uid_3rd, true);

		if (dtex_gl_out_of_memory()) {
			// return 1 tex
			--buf->end_tex;
			dtex_gl_release_texture(buf->tex_pool[buf->end_tex].gl_id, 0);
			buf->tex_pool[buf->end_tex].gl_id = 0;
			buf->tex_pool[buf->end_tex].uid_3rd = 0;
			break;
		}

        buf->tex_pool[i].gl_id = gl_id;
		buf->tex_pool[i].uid_3rd = uid_3rd;

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
	for (int i = 0; i < buf->end_tex; ++i) {
		dtex_gl_release_texture(buf->tex_pool[i].gl_id, 0);
	}
	buf->next_tex = buf->end_tex = 0;

	for (int i = 0; i < buf->target_size; ++i) {
		dtex_del_target(buf->target_pool[i]);
	}
	buf->target_size = 0;

	free(buf);
}

void
dtexbuf_fetch_texid(struct dtex_buffer* buf, int* gl_id, int* uid_3rd) {
	*gl_id = 0;
	*uid_3rd = 0;
	if (buf->next_tex < buf->end_tex) {
		*gl_id = buf->tex_pool[buf->next_tex].gl_id;
		*uid_3rd = buf->tex_pool[buf->next_tex].uid_3rd;
		++buf->next_tex;
	}
}

bool 
dtexbuf_return_texid(struct dtex_buffer* buf, int gl_id, int uid_3rd) {
	if (buf->next_tex > 0) {
		--buf->next_tex;
		buf->tex_pool[buf->next_tex].gl_id = gl_id;
		buf->tex_pool[buf->next_tex].uid_3rd = uid_3rd;
        return true;
	} else if (buf->end_tex < MAX_TEX_COUNT - 1) {
		buf->tex_pool[buf->end_tex].gl_id = gl_id;
		buf->tex_pool[buf->end_tex].uid_3rd = uid_3rd;
		buf->end_tex++;
        return true;
	} else {
		return false;
	}
}

int 
dtexbuf_get_tex_edge(struct dtex_buffer* buf) {
	return buf->tex_edge;
}

struct dtex_target* 
dtex_buf_fetch_target(struct dtex_buffer* buf) {
	if (buf->target_size == 0) {
		return dtex_new_target();
	} else {
		struct dtex_target* target = buf->target_pool[buf->target_size - 1];
		--buf->target_size;
		return target;
	}
}

void 
dtex_buf_return_target(struct dtex_buffer* buf, struct dtex_target* target) {
	if (buf->target_size == MAX_FBO_COUNT) {
		dtex_del_target(target);
	} else {
		buf->target_pool[buf->target_size++] = target;
	}
}