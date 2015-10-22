#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_render_target_h
#define dynamic_texture_render_target_h

struct dtex_texture;

void dtex_render_init();

void dtex_draw_to_texture(struct dtex_texture* src, struct dtex_texture* dst, const float vb[16]);
void dtex_draw_finish();

#endif // dynamic_texture_render_target_h

#ifdef __cplusplus
}
#endif