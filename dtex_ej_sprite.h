#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_ejoy2d_sprite_h
#define dynamic_texture_ejoy2d_sprite_h

#include "ejoy2d.h"

struct dtex_package;
struct dtex_c2;

struct ej_sprite* dtex_ej_sprite_create(struct dtex_package*, int spr_id);

void dtex_ej_sprite_draw(struct dtex_package*, struct dtex_c2*, struct ej_sprite*, struct ej_srt*);

#endif // dynamic_texture_ejoy2d_sprite_h

#ifdef __cplusplus
}
#endif