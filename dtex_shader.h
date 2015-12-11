#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_shader_h
#define dynamic_texture_shader_h

#define DTEX_PROGRAM_NULL	0
#define DTEX_PROGRAM_NORMAL	1
#define DTEX_PROGRAM_ETC1	2
#define DTEX_PROGRAM_SHAPE	3

void dtex_shader_init(void (*program)(int n),
					  void (*blend)(int mode),
					  void (*set_texture)(int id),
					  int (*get_texture)(),
					  void (*set_target)(int id),
					  int (*get_target)(),
					  void (*draw_begin)(),
					  void (*draw)(const float vb[16]),
					  void (*draw_end)(),
					  void (*draw_flush)());

void dtex_shader_program(int n);
void dtex_shader_blend(int mode);

void dtex_shader_set_texture(int id);
int dtex_shader_get_texture();

void dtex_shader_set_target(int id);
int dtex_shader_get_target();

void dtex_shader_begin();
void dtex_shader_draw(const float vb[16]);
//void dtex_shader_draw_triangle(const float* vb, int count);
void dtex_shader_end();

void dtex_shader_flush();

#endif // dynamic_texture_shader_h

#ifdef __cplusplus
}
#endif