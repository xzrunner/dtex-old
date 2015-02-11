#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_packer_h
#define dynamic_texture_packer_h

#include "dtex_typedef.h"

struct dp_pos {
	struct dtex_rect r;
	bool is_rotated;

	void* ud;
};

struct dtex_packer;

struct dtex_packer* dtexpacker_create(int width, int height, int size);
void dtexpacker_release(struct dtex_packer*);

struct dp_pos* dtexpacker_add(struct dtex_packer*, int width, int height, bool can_rotate);

void dtexpacker_get_size(struct dtex_packer*, int* width, int* height);

#endif // dynamic_texture_packer_h

#ifdef __cplusplus
}
#endif