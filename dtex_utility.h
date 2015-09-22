#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_utility_h
#define dynamic_texture_utility_h

#include <stdint.h>
#include <stdbool.h>

struct dtex_img_pos;
struct dtex_rect;
struct dtex_c2;
struct dtex_inv_size;

struct int_array {
	int* data;
	int size;
};
struct int_array* dtex_get_picture_id_set(struct ej_package*, int id);

void dtex_relocate_spr(struct ej_package*, struct int_array* array, int tex_idx, struct dtex_img_pos* src, struct dtex_img_pos* dst);

void dtex_relocate_c2_key(struct dtex_c2*, struct ej_package*, struct int_array* array, struct dtex_img_pos* src, struct dtex_img_pos* dst);

void dtex_relocate_pic_part(uint16_t part_src[8], struct dtex_inv_size* src_sz, struct dtex_rect* src_rect, 
	struct dtex_inv_size* dst_sz, struct dtex_rect* dst_rect, int rotate, float trans_vb[16], float dst_vb[8]);

void dtex_get_pic_src_rect(uint16_t* src, struct dtex_rect* rect);

#define STREAM_IMPORT(ptr, data)			\
	memcpy(&(data), (ptr), sizeof(data));	\
	(ptr) += sizeof(data);					\

#endif // dynamic_texture_utility_h

#ifdef __cplusplus
}
#endif