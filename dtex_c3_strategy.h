#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_c3_strategy_h
#define dynamic_texture_c3_strategy_h

#include <stdbool.h>

struct dtex_c3_strategy;

struct dtex_c3_strategy* dtex_c3_strategy_create(bool is_static);
void dtex_c3_strategy_release(struct dtex_c3_strategy*);

bool dtex_c3_is_static(struct dtex_c3_strategy*);

#endif // dynamic_texture_c3_strategy_h

#ifdef __cplusplus
}
#endif