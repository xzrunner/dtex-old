#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_cache2_h
#define dynamic_texture_cache2_h

#include <stdbool.h>

struct dtex_c2;
struct dtex_loader;
struct dtex_rect;
struct dtex_tp_pos;
struct dtex_texture;
struct dtex_texture_with_rect;
struct dtex_package;

struct dtex_c2* dtex_c2_create(int texture_size, bool one_tex, int static_count);
void dtex_c2_release(struct dtex_c2*);

void dtex_c2_clear(struct dtex_c2*, struct dtex_loader*);

void dtex_c2_load_begin(struct dtex_c2*);
void dtex_c2_load(struct dtex_c2*, struct dtex_package* pkg, int spr_id);
void dtex_c2_load_end(struct dtex_c2*, struct dtex_loader*);

float* dtex_c2_lookup_texcoords(struct dtex_c2*, int pkg_id, int spr_id, float vb[8], int* out_texid);
void dtexc2_lookup_node(struct dtex_c2*, int pkg_id, int spr_id, struct dtex_texture** out_tex, struct dtex_tp_pos** out_pos);

void dtex_c2_debug_draw(struct dtex_c2*);

#endif // dynamic_texture_cache2_h

#ifdef __cplusplus
}
#endif