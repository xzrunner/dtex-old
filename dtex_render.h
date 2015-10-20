#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_render_target_h
#define dynamic_texture_render_target_h

void dtex_render_before();
void dtex_render_after();

void dtex_draw_to_texture(struct dtex_texture* src, struct dtex_texture* dst, const float vb[16]);

#endif // dynamic_texture_render_target_h

#ifdef __cplusplus
}
#endif