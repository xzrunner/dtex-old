#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_screen_h
#define dynamic_texture_screen_h

#define SCALE 16

void dtex_set_screen(float w, float h, float scale);
void dtex_get_screen(float* w, float* h, float* scale);

void dtex_screen_trans(float* x, float* y);

#endif // dynamic_texture_screen_h

#ifdef __cplusplus
}
#endif