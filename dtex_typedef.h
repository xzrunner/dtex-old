#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_typedef_h
#define dynamic_texture_typedef_h

#include <stdint.h>
#include <stdbool.h>

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