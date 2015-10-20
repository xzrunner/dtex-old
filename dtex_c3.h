#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_cache3_h
#define dynamic_texture_cache3_h

#include <stdbool.h>

struct dtex_c3;
struct dtex_package;
struct dtex_loader;
struct dtex_rect;

struct dtex_c3* dtex_c3_create(int texture_size, bool one_tex_mode);
void dtex_c3_release(struct dtex_c3*);

void dtex_c3_load(struct dtex_c3*, struct dtex_package*, float scale, bool force);
void dtex_c3_load_end(struct dtex_c3*, struct dtex_loader*, bool async);

//// todo cache and sort
//struct dtex_tp_pos* dtex_c3_load_tex(struct dtex_c3*, struct dtex_texture*, struct dtex_texture** dst);

void dtex_c3_query_map_info(struct dtex_c3*, struct dtex_package*, struct dtex_texture** textures, struct dtex_rect** regions);

void dtex_c3_debug_draw(struct dtex_c3*);

#endif // dynamic_texture_cache3_h

#ifdef __cplusplus
}
#endif