#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_statistics_h
#define dynamic_texture_statistics_h

#include <stdbool.h>

void dtex_stat_init();

void dtex_stat_draw_start();
void dtex_stat_draw_end();

void dtex_stat_add_drawcall();
int dtex_stat_get_drawcall();

struct stat_texture {
	int id;
	int w, h;
};

void dtex_stat_add_texture(int texid, int width, int height);
void dtex_stat_delete_texture(int texid);
void dtex_stat_get_texture(int* count, struct stat_texture** list);

#endif // dynamic_texture_statistics_h

#ifdef __cplusplus
}
#endif