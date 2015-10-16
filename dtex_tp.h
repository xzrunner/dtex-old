#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_texture_packer_h
#define dynamic_texture_texture_packer_h

#include "dtex_typedef.h"

struct dtex_tp_pos {
	struct dtex_rect r;
	bool is_rotated;

	void* ud;
};

struct dtex_tp;

struct dtex_tp* dtex_tp_create(int width, int height, int size);
void dtex_tp_release(struct dtex_tp*);

void dtex_tp_clear(struct dtex_tp*);

struct dtex_tp_pos* dtex_tp_add(struct dtex_tp*, int width, int height, bool can_rotate);

void dtex_tp_get_size(struct dtex_tp*, int* width, int* height);

int dtex_tp_get_free_space(struct dtex_tp*);

#endif // dynamic_texture_texture_packer_h

#ifdef __cplusplus
}
#endif