#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_buffer_h
#define dynamic_texture_buffer_h

#include "stdbool.h"

struct dtex_buffer;

struct dtex_buffer* dtexbuf_create();
void dtexbuf_release(struct dtex_buffer*);

int dtexbuf_reserve(struct dtex_buffer*, int area_need);

unsigned int dtexbuf_fetch_texid(struct dtex_buffer*);
bool dtexbuf_return_texid(struct dtex_buffer*, unsigned int texid);
int dtexbuf_get_tex_edge(struct dtex_buffer*);

struct dtex_target* dtex_buf_fetch_target(struct dtex_buffer*);
void dtex_buf_return_target(struct dtex_buffer*, struct dtex_target*);

#endif // dynamic_texture_buffer_h

#ifdef __cplusplus
}
#endif