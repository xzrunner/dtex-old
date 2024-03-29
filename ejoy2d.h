#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_ejoy2d_h
#define dynamic_texture_ejoy2d_h

#include <matrix.h>
#include <spritepack.h>
#ifdef EASY_EDITOR
#include <ej_sprite.h>
#else
#include <sprite.h>
#endif // EASY_EDITOR

// "matrix.h"
#define ej_srt						srt
#define ej_matrix					matrix
#define ej_matrix_srt				matrix_srt
#define ej_matrix_identity			matrix_identity

// "spritepack.h"
#define ej_sprite_pack				sprite_pack
#define ej_pack_quad				pack_quad
#define ej_pack_picture				pack_picture
#define ej_pack_animation			pack_animation
#define ej_pack_frame				pack_frame
#define ej_sprite_trans				sprite_trans

// "sprite.h"
#define ej_sprite					sprite
#define ej_sprite_setframe			sprite_setframe
#define ej_sprite_size				sprite_size
#define ej_sprite_init				sprite_init
#define ej_sprite_trans_mul			sprite_trans_mul
#define ej_sprite_component			sprite_component
#define ej_sprite_childname			sprite_childname

#endif // dynamic_texture_ejoy2d_h

#ifdef __cplusplus
}
#endif