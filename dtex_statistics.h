#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_statistics_h
#define dynamic_texture_statistics_h

#include <stdbool.h>

void dtex_stat_init();

// texture
void dtex_stat_add_tex(int texid, int type, int width, int height);
void dtex_stat_del_tex(int texid);
void dtex_stat_dump_tex();
int  dtex_stat_tex_mem();

#endif // dynamic_texture_statistics_h

#ifdef __cplusplus
}
#endif