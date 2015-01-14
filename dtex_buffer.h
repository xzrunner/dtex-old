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

struct dtex_fbo* dtexbuf_fetch_fbo(struct dtex_buffer*);
void dtexbuf_return_fbo(struct dtex_buffer*, struct dtex_fbo*);

#endif // dynamic_texture_buffer_h