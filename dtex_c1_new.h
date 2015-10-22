#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_cache1_h
#define dynamic_texture_cache1_h

#include "ejoy2d.h"

struct dtex_c1;
struct dtex_c2;
struct dtex_package;

struct dtex_c1* dtex_c1_create(int texture_size);
void dtex_c1_release(struct dtex_c1*);

void dtex_c1_update(struct dtex_c1*, struct dtex_c2*, struct dtex_package*, struct ej_sprite* spr);

void dtex_c1_debug_draw(struct dtex_c1*);

#endif // dynamic_texture_cache1_h

#ifdef __cplusplus
}
#endif