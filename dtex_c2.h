#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_cache2_h
#define dynamic_texture_cache2_h

#include "ejoy2d.h"

#include <stdbool.h>

struct dtex_c2;
struct dtex_loader;
struct dtex_rect;
struct dtex_tp_pos;

struct dtex_c2* dtex_c2_create(int texture_size);
void dtex_c2_release(struct dtex_c2*);

void dtex_c2_clear(struct dtex_c2*, struct dtex_loader*);

void dtex_c2_load_begin(struct dtex_c2*);
void dtex_c2_load(struct dtex_c2*, struct dtex_package* pkg, int spr_id);
void dtex_c2_load_end(struct dtex_c2*, struct dtex_loader*, bool use_only_one_texture);

float* dtex_c2_lookup_texcoords(struct dtex_c2*, struct dtex_texture* tex, float vb[8], int* out_texid);
void dtexc2_lookup_node(struct dtex_c2*, int texid, struct dtex_rect* rect, struct dtex_texture** out_tex, struct dtex_tp_pos** out_pos);

void dtex_c2_change_key(struct dtex_c2*, struct dtex_texture_with_rect* src, struct dtex_texture_with_rect* dst);

void dtex_c2_debug_draw(struct dtex_c2*);

#endif // dynamic_texture_cache2_h

#ifdef __cplusplus
}
#endif