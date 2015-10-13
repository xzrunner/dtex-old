#include "dtex_hard_res.h"
#include "dtex_gl.h"

#include <opengl.h>

#include <stdlib.h>
#include <string.h>

struct res {
	int max_texture_size;
	int max_texture_area, remain_texture_area;
};

static struct res R;

static inline void
_init_max_texture_size() {
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &R.max_texture_size);
}

static inline void
_init_max_texture_area(int need_texture_area) {
	int edge = 1024;
	int area = edge * edge;
	uint8_t* empty_data = (uint8_t*)malloc(area * 4);
	memset(empty_data, 0x00, area * 4);

	glActiveTexture(GL_TEXTURE0);

	int max_count = ceil(need_texture_area / area);
	unsigned int id_list[max_count];

	int curr_area = 0;
	int curr_count = 0;
	while (true) {
		unsigned int texid = 0;
		glGenTextures(1, &texid);
		glBindTexture(GL_TEXTURE_2D, texid);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)edge, (GLsizei)edge, 0, GL_RGBA, GL_UNSIGNED_BYTE, empty_data);
		if (dtex_gl_out_of_memory()) {
			break;			
		} else {
			if (curr_area >= need_texture_area) {
				break;
			}
			id_list[curr_count++] = texid;
			curr_area += area;
		}
	}

	glDeleteTextures(curr_count, id_list);

	glBindTexture(GL_TEXTURE_2D, 0);

	free(empty_data);

	R.remain_texture_area = R.max_texture_area = curr_area;
}

void 
dtex_hard_res_init(int need_texture_area) {
	_init_max_texture_size();
	_init_max_texture_area(need_texture_area);
}

int 
dtex_max_texture_size() {
	return R.max_texture_size;
}

//bool dtex_hard_res_fetch_texture(int edge) {
//	int area = edge * edge;
//	if (R.remain_texture_area < area) {
//		return false;
//	} else {
//		R.remain_texture_area -= area;
//		return true;
//	}
//}
//
//void dtex_hard_res_return_texture(int edge) {
//	int area = edge * edge;
//	R.remain_texture_area += area;
//}