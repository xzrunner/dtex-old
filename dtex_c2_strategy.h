#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_c2_strategy_h
#define dynamic_texture_c2_strategy_h

#include "ejoy2d.h"

struct dtex_c2_strategy;

struct dtex_c2_strategy* dtex_c2_strategy_create(int n);
void dtex_c2_strategy_release(struct dtex_c2_strategy*);

void dtex_c2_on_draw_sprite(struct ej_sprite* spr);

void dtex_c2_strategy_clear(struct dtex_package*);

#endif // dynamic_texture_c2_strategy_h

#ifdef __cplusplus
}
#endif