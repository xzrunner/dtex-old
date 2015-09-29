#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_packer_ext_h
#define dynamic_texture_packer_ext_h

#include <stdint.h>

struct dp_rect {
	int16_t w, h;
	void* ud;

//	struct dtex_packer* dst_packer;
	int dst_packer_idx;
	struct dp_pos* dst_pos;
};

struct dtex_array* dtex_packer_square_multi(struct dp_rect** rects, int sz);

#endif // dynamic_texture_packer_ext_h

#ifdef __cplusplus
}
#endif