#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_typedef_h
#define dynamic_texture_typedef_h

#include <stdint.h>
#include <stdbool.h>

// todo remove
#define DTEX_PNG4	0
#define DTEX_PNG8	1
#define DTEX_PVR	2
#define DTEX_ETC1	3
#define DTEX_ETC2	4

enum FILE_FORMAT {
	FILE_INVALID = 0,
	FILE_EPT,
	FILE_EPE,
	FILE_RRP,
	FILE_PTS,
	FILE_RRR,
	FILE_B4R
};

enum DTEX_TF_FMT {
	DTEX_TF_INVALID = 0,
	DTEX_TF_RGBA8,
	DTEX_TF_RGBA4,
	DTEX_TF_PVR2,
	DTEX_TF_PVR4,
	DTEX_TF_ETC1,
	DTEX_TF_ETC2,
};

enum PIXEL_FORMAT {
	PIXEL_INVALID = 0,
	PIXEL_ALPHA,
	PIXEL_RGB,
	PIXEL_RGBA,
	PIXEL_LUMINANCE,
	PIXEL_LUMINANCE_ALPHA
};

#define QUAD_TEXID_IN_PKG_MAX 1024

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

#endif // dynamic_texture_typedef_h

#ifdef __cplusplus
}
#endif