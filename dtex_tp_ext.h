#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_texture_packer_ext_h
#define dynamic_texture_texture_packer_ext_h

#include <stdint.h>

struct dtex_tp_rect {
	int16_t w, h;
	void* ud;

//	struct dtex_tp* dst_packer;
	int dst_packer_idx;
	struct dtex_tp_pos* dst_pos;
};

struct ds_array* dtex_packer_square_multi(struct dtex_tp_rect** rects, int sz);

#endif // dynamic_texture_texture_packer_ext_h

#ifdef __cplusplus
}
#endif