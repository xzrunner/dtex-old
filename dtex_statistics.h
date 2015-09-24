#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_statistics_h
#define dynamic_texture_statistics_h

#include <stdbool.h>

void dtex_stat_draw_start();
void dtex_stat_draw_end();

void dtex_add_drawcall();
int dtex_get_drawcall();

#endif // dynamic_texture_statistics_h

#ifdef __cplusplus
}
#endif