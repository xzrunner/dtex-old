#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_cache1_h
#define dynamic_texture_cache1_h

#include <stdbool.h>

struct dtex_c1;
struct dtex_buffer;
struct draw_params;

struct ej_sprite_pack;
struct animation;

struct dtex_c1* dtexc1_create(struct dtex_buffer*);
void dtexc1_release(struct dtex_c1*, struct dtex_buffer*);

void dtexc1_load_anim(struct dtex_c1*, struct ej_sprite_pack* pkg, struct animation* ani, int action);
bool dtexc1_draw_anim(struct dtex_c1*, struct ej_sprite_pack* pkg, struct animation* ani, int action, int frame, struct draw_params* params);

void dtexc1_debug_draw(struct dtex_c1*);

#endif // dynamic_texture_cache1_h

#ifdef __cplusplus
}
#endif