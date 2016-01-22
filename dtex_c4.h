#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_cache4_h
#define dynamic_texture_cache4_h

#include <stdbool.h>

struct dtex_c4;
struct dtex_package;
struct dtex_loader;
struct dtex_rect;
struct dtex_texture;

struct dtex_c4* dtex_c4_create(int tex_size, int tex_count);
void dtex_c4_release(struct dtex_c4*);

void dtex_c4_clear(struct dtex_c4*);

void dtex_c4_load(struct dtex_c4*, struct dtex_package*);
void dtex_c4_load_end(struct dtex_c4*, struct dtex_loader*, bool async);

void dtex_c4_debug_draw(struct dtex_c4*);

#endif // dynamic_texture_cache4_h

#ifdef __cplusplus
}
#endif