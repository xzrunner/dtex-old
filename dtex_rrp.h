#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_regular_rect_packer_h
#define dynamic_texture_regular_rect_packer_h

#include <stdint.h>
#include <stdbool.h>

#define EXPORT_RRP

struct dr_rect {
	int32_t texid;
	int16_t x, y;
	int16_t w, h;
};

struct dr_part {
	struct dr_rect src, dst;
	bool is_rotated;
};

struct dr_picture {
	int16_t id;
	int16_t w, h;

	int16_t part_sz;
	struct dr_part* part;

	// todo triangle trips
};

struct dtex_rrp;

struct dtex_rrp* dtex_rrp_create(void* data, int sz, int cap);
void dtex_rrp_release(struct dtex_rrp*);

struct dr_picture* dtex_rrp_get_pic(struct dtex_rrp*, int id);

#ifdef EXPORT_RRP
size_t dtex_rrp_size(void* data, int sz);
#endif // EXPORT_RRP

#endif // dynamic_texture_regular_rect_packer_h

#ifdef __cplusplus
}
#endif