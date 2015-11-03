#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_gl_h
#define dynamic_texture_gl_h

#ifndef USED_IN_EDITOR
#include "ejoy2d.h"
#endif // USED_IN_EDITOR

#include <stdbool.h>

#ifndef USED_IN_EDITOR
void dtex_gl_init(struct ej_render* R);
#endif // USED_IN_EDITOR

void dtex_gl_create_texture(int type, int width, int height, const void* data, int channel, unsigned int* gl_id, int* uid_3rd, bool create_by_ej);
void dtex_gl_release_texture(unsigned int id, int channel);

void dtex_gl_update_subtex(const void* pixels, int x, int y, int w, int h, unsigned int id);

#ifndef USED_IN_EDITOR
void dtex_release_ej_texture(int uid_3rd);
#endif // USED_IN_EDITOR

void dtex_gl_clear_color(float r, float g, float b, float a);

void dtex_gl_viewport(int x, int y, int w, int h);

void dtex_gl_scissor(int x, int y, int w, int h);

void dtex_gl_finish();

void dtex_gl_bind_vertex_array(int id);

bool dtex_gl_out_of_memory();

// debug
bool dtex_gl_is_texture(unsigned int id);
int dtex_gl_get_curr_texrute();
int dtex_gl_get_curr_target();

void dtex_gl_check_error();

#endif // dynamic_texture_gl_h

#ifdef __cplusplus
}
#endif