#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_ejoy2d_h
#define dynamic_texture_ejoy2d_h

#include <matrix.h>
#include <spritepack.h>
#include <sprite.h>
#include <shader.h>
#ifndef USED_IN_EDITOR
#include <render.h>
#include <renderbuffer.h>
#endif // USED_IN_EDITOR

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
#define ej_pack_particle3d			pack_particle3d
#define ej_pack_particle2d			pack_particle2d
#define ej_sprite_trans				sprite_trans

// "sprite.h"
#define ej_sprite					sprite
#define ej_sprite_setframe			sprite_setframe
#define ej_sprite_size				sprite_size
#define ej_sprite_init				sprite_init
#define ej_sprite_trans_mul			sprite_trans_mul
#define ej_sprite_component			sprite_component
#define ej_sprite_childname			sprite_childname
#define ej_sprite_mount				sprite_mount

#ifndef USED_IN_EDITOR

// "render.h"
#define ej_render					render
#define EJ_TEXTURE_RGBA8			TEXTURE_RGBA8
#define EJ_TEXTURE					TEXTURE
#define ej_render_texture_create	render_texture_create
#define ej_render_texture_update	render_texture_update
#define ej_render_get_texture_gl_id	render_get_texture_gl_id
#define ej_render_release			render_release

// "renderbuffer.h"
#define ej_vertex_pack				vertex_pack

// "shader.h"
#define ej_shader_texture			shader_texture
#define ej_shader_program			shader_program
#define ej_shader_draw				shader_draw

#endif // USED_IN_EDITOR

#endif // dynamic_texture_ejoy2d_h

#ifdef __cplusplus
}
#endif