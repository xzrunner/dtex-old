#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_cache_screen_h
#define dynamic_texture_cache_screen_h

struct dtex_cs;

struct dtex_cs* dtex_cs_create();
void dtex_cs_release(struct dtex_cs*);

void dtex_cs_on_size(struct dtex_cs* cs, int width, int height);

void dtex_cs_bind(struct dtex_cs*);
void dtex_cs_unbind(struct dtex_cs*);

void dtex_cs_draw_to_screen(struct dtex_cs*);

void dtex_cs_debug_draw(struct dtex_cs*);

#endif // dynamic_texture_cache_screen_h

#ifdef __cplusplus
}
#endif