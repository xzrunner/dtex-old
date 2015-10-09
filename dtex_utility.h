#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_utility2_h
#define dynamic_texture_utility2_h

#include "ejoy2d.h"

struct dtex_array* dtex_get_picture_id_unique_set(struct ej_sprite_pack*, int* spr_ids, int spr_count);

struct dtex_array* dtex_get_texture_id_unique_set(struct ej_sprite_pack*, int* spr_ids, int spr_count);

#endif // dynamic_texture_utility2_h

#ifdef __cplusplus
}
#endif