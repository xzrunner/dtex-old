#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_gl_h
#define dynamic_texture_gl_h

#define USE_EJ_RENDER

#ifdef USE_EJ_RENDER
#include "ejoy2d.h"
#endif // USE_EJ_RENDER

#include <stdbool.h>

#ifdef USE_EJ_RENDER
void dtex_gl_init(struct ej_render* R);
#endif // USE_EJ_RENDER

void dtex_gl_create_texture(int type, int width, int height, const void* data, int channel, int* gl_id, int* uid_3rd);
void dtex_gl_release_texture(unsigned int id, int channel);

void dtex_gl_clear_color(float r, float g, float b, float a);

void dtex_gl_viewport(int x, int y, int w, int h);

// load
int dtex_gl_get_max_texture_size();
bool dtex_gl_out_of_memory();

// debug
bool dtex_gl_istexture(unsigned int id);

#endif // dynamic_texture_gl_h

#ifdef __cplusplus
}
#endif