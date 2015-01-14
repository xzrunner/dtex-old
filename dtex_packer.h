#ifndef dynamic_texture_packer_h
#define dynamic_texture_packer_h

#include "dtex_typedef.h"

struct dp_position {
	struct dtex_rect r;
	int16_t is_rotated;

	void* ud;
};

struct dtex_packer;

struct dtex_packer* dtexpacker_create(int width, int height, int size);
void dtexpacker_release(struct dtex_packer* packer);
struct dp_position* dtexpacker_add(struct dtex_packer* packer, int width, int height);

#endif // dynamic_texture_packer_h