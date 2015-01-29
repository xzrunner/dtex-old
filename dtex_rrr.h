#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_regular_rect_raw_h
#define dynamic_texture_regular_rect_raw_h

#include <stdint.h>

#ifdef _MSC_VER
#define EXPORT_RRR
#endif // _MSC_VER

struct rrr_part {
	int16_t x, y;
	int16_t w, h;

	uint64_t* data;
};

struct rrr_picture {
	int16_t id;
	int16_t w, h;

	int16_t part_sz;
	struct rrr_part* part;
};

struct dtex_rrr;

struct dtex_rrr* dtex_rrr_create(void* data, int sz, int cap);
void dtex_rrr_release(struct dtex_rrr*);

#ifdef EXPORT_RRR
size_t dtex_rrr_size(void* data, int sz);
#endif // EXPORT_RRR

#endif // dynamic_texture_regular_rect_raw_h

#ifdef __cplusplus
}
#endif