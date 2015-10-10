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