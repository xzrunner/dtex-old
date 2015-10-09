#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_ejoy2d_utility_h
#define dynamic_texture_ejoy2d_utility_h

#include "ejoy2d.h"

void dtex_ej_pkg_traverse(struct ej_sprite_pack* ej_pkg, void (*pic_func)(int pic_id, struct ej_pack_picture* ej_pic, void* ud), void* ud);

void dtex_ej_spr_traverse(struct ej_sprite_pack* ej_pkg, int spr_id, void (*pic_func)(int pic_id, struct ej_pack_picture* ej_pic, void* ud), void* ud);

#endif // dynamic_texture_ejoy2d_utility_h

#ifdef __cplusplus
}
#endif