#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_cache1_h
#define dynamic_texture_cache1_h

#include <stdint.h>

struct dtex_c1;

struct dtex_c1* dtex_c1_create(int texture_size);
void dtex_c1_release(struct dtex_c1*);

void dtex_c1_bind(struct dtex_c1*);
void dtex_c1_unbind(struct dtex_c1*);

//void dtex_c1_update(struct dtex_c1*, struct dtex_c2*, struct dtex_package*, struct ej_sprite* spr);

uint32_t dtex_c1_get_texture_id(struct dtex_c1*);
uint32_t dtex_c1_get_texture_size(struct dtex_c1*);

void dtex_c1_draw(struct dtex_c1*, float src_w, float src_h, float dst_w, float dst_h);

void dtex_c1_debug_draw(struct dtex_c1*);

#endif // dynamic_texture_cache1_h

#ifdef __cplusplus
}
#endif