#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_cache2_h
#define dynamic_texture_cache2_h

#include <ejoy2d.h>

#include <stdbool.h>

struct dtex_c2;
struct dtex_buffer;
struct dtex_loader;
struct dtex_rect;
struct dtex_texture;
struct dp_pos;

struct dtex_c2* dtex_c2_create(struct dtex_buffer*);
void dtex_c2_release(struct dtex_c2*, struct dtex_buffer*);

void dtex_c2_load_begin(struct dtex_c2*);
void dtex_c2_load(struct dtex_c2*, struct dtex_package* pkg, int spr_id, int tex_idx);
// todo preload no name spr
void dtex_c2_load_end(struct dtex_c2*, struct dtex_buffer*, struct dtex_loader*, bool use_only_one_texture);

float* dtexc2_lookup_texcoords(struct dtex_c2*, int texid, struct dtex_rect* rect, int* out_texid);
void dtexc2_lookup_node(struct dtex_c2*, int texid, struct dtex_rect* rect, struct dtex_texture** out_tex, struct dp_pos** out_pos);

void dtexc2_change_key(struct dtex_c2*, int src_texid, struct dtex_rect* src_rect, int dst_texid, struct dtex_rect* dst_rect);

void dtexc2_debug_draw(struct dtex_c2*);

#endif // dynamic_texture_cache2_h

#ifdef __cplusplus
}
#endif