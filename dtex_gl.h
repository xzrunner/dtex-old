#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_gl_h
#define dynamic_texture_gl_h

#include <stdbool.h>

void dtex_gl_init(void (*clear_color)(float xmin, float ymin, float xmax, float ymax));
void dtex_gl_clear_color2(float xmin, float ymin, float xmax, float ymax);

void dtex_gl_texture_init(int (*texture_create)(int type, int width, int height, const void* data, int channel, unsigned int id),
						  void (*texture_release)(int id),
						  void (*texture_update)(const void* pixels, int w, int h, unsigned int id),
						  int (*texture_id)(int id));

int dtex_gl_create_texture(int type, int width, int height, const void* data, int channel, unsigned int id);
void dtex_gl_release_texture(int id);
void dtex_gl_update_texture(const void* pixels, int w, int h, unsigned int id);
int dtex_gl_texture_id(int id);

void dtex_gl_clear_color(float r, float g, float b, float a);

void dtex_gl_viewport(int x, int y, int w, int h);

void dtex_gl_scissor(int x, int y, int w, int h);

//// for debug
//void dtex_gl_finish();

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