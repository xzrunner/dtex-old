#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_typedef_h
#define dynamic_texture_typedef_h

#include <stdint.h>
#include <stdbool.h>

// todo remove
#define TEXTURE4 0
#define TEXTURE8 1
//#define DETAIL 2
#define PVRTC  3
//#define KTX 4
//#define PKM 5
#define PKMC 6
//#define RRP 11
//#define PTS 12
//#define RRR 13
//#define B4R 14

#define FILE_EPT 0
#define FILE_EPE 1
#define FILE_RRP 2
#define FILE_PTS 3
#define FILE_RRR 4
#define FILE_B4R 5

#define QUAD_TEXID_IN_PKG_MAX 1024

struct draw_params {
	int x, y;
	int* mat;
	bool mirror;
	struct screen_coord* screen;
	uint32_t color, additive;
};

// struct dtex_pos {
// 	int16_t x, y;
// };

struct dtex_rect {
	int16_t xmin, ymin;
	int16_t xmax, ymax;
};

// struct dtex_rect2 {
// 	int16_t x, y;
// 	int16_t w, h;
// };

// struct dtex_size {
// 	int16_t w, h;
// };

struct dtex_inv_size {
	float inv_w, inv_h;
};

bool dtex_rect_same(struct dtex_rect* r0, struct dtex_rect* r1);

struct dtex_img_pos {
	unsigned int id, id_alpha;
	float inv_width, inv_height;

	struct dtex_rect rect;
//	bool is_rotated;
};

#endif // dynamic_texture_typedef_h

#ifdef __cplusplus
}
#endif