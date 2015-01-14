#ifndef dynamic_texture_utility_h
#define dynamic_texture_utility_h

#include <stdint.h>
#include <stdbool.h>

struct dtex_pos;
struct dtex_rect;
struct dtex_c2;
struct dtex_inv_size;

struct ej_package;

void dtex_relocate_spr(struct ej_package*, int id, struct dtex_pos* src, struct dtex_pos* dst);

void dtex_relocate_c2_key(struct dtex_c2*, struct ej_package*, int id, struct dtex_pos* src, struct dtex_pos* dst);

void dtex_relocate_pic_part(int16_t part_src[8], struct dtex_inv_size* src_sz, struct dtex_rect* src_rect, 
	struct dtex_inv_size* dst_sz, struct dtex_rect* dst_rect, bool rotate, float trans_vb[16], float dst_vb[8]);

void dtex_get_pic_src_rect(int16_t* src, struct dtex_rect* rect);

#endif // dynamic_texture_utility_h