#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_shader_h
#define dynamic_texture_shader_h

#define PROGRAM_NULL	0
#define PROGRAM_NORMAL	1
#define PROGRAM_ETC1	2
#define PROGRAM_SHAPE	3

void dtex_shader_load();
void dtex_shader_unload();

void dtex_shader_program(int n);
void dtex_shader_blend(int mode);

void dtex_shader_set_texture(int id);
int dtex_shader_get_texture();

void dtex_shader_set_target(int id);
int dtex_shader_get_target();

void dtex_shader_draw(const float vb[16]);
void dtex_shader_draw_triangle(const float* vb, int count);

void dtex_shader_flush();

#endif // dynamic_texture_shader_h

#ifdef __cplusplus
}
#endif