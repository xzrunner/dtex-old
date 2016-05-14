#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_cache_screen_h
#define dynamic_texture_cache_screen_h

#include <stdbool.h>
#include <stdint.h>

struct dtex_cs;

struct dtex_cs_rect {
	float xmin, ymin;
	float xmax, ymax;
};

struct dtex_cs* dtex_cs_create();
void dtex_cs_release(struct dtex_cs*);

void dtex_cs_on_size(struct dtex_cs* cs, int width, int height);

void dtex_cs_bind(struct dtex_cs*);
void dtex_cs_unbind(struct dtex_cs*);

void dtex_cs_add_inv_rect(struct dtex_cs*, struct dtex_cs_rect* rect);

void dtex_cs_traverse(struct dtex_cs*, void (*cb)(struct dtex_cs_rect* r, void* ud), void* ud);

void dtex_cs_set_pos(struct dtex_cs*, float x, float y, float scale);

bool dtex_cs_dirty(struct dtex_cs*);

void dtex_cs_draw(struct dtex_cs*, void (*before_draw)(void* ud), void* ud);

void dtex_cs_reload(struct dtex_cs*);

uint32_t dtex_cs_get_texture_id(struct dtex_cs*);

void dtex_cs_debug_draw(struct dtex_cs*);

#endif // dynamic_texture_cache_screen_h

#ifdef __cplusplus
}
#endif