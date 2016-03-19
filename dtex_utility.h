#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_utility2_h
#define dynamic_texture_utility2_h

#include "ejoy2d.h"

struct ds_array;

void dtex_get_picture_id_unique_set(struct ej_sprite_pack*, int* spr_ids, int spr_count, struct ds_array* uni_set);

void dtex_get_texture_id_unique_set(struct ej_sprite_pack*, int* spr_ids, int spr_count, struct ds_array* uni_set);

#endif // dynamic_texture_utility2_h

#ifdef __cplusplus
}
#endif