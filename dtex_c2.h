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
struct dtex_cg;

struct dtex_c2* dtex_c2_create(int width, int height, bool one_tex, int static_count, bool open_cg, int src_extrude);
void dtex_c2_release(struct dtex_c2*);

void dtex_c2_clear(struct dtex_c2*);
void dtex_c2_clear_from_cg(struct dtex_c2*, struct dtex_loader*);

int dtex_c2_get_cg_quad();
void dtex_c2_set_static_quad(struct dtex_c2*, int quad);

void dtex_c2_load_begin(struct dtex_c2*);
void dtex_c2_load_spr(struct dtex_c2*, struct dtex_package* pkg, int spr_id);
void dtex_c2_load_tex(struct dtex_c2*, int tex_id, int tex_width, int tex_height, int key);
void dtex_c2_load_end(struct dtex_c2*, struct dtex_loader*);

void dtex_c2_remove_tex(struct dtex_c2*, int key);

void dtex_c2_reload_begin(struct dtex_c2*);
void dtex_c2_reload_tex(struct dtex_c2*, int tex_id, int tex_width, int tex_height, int key);
void dtex_c2_reload_end();

float* dtex_c2_query_spr(struct dtex_c2*, int pkg_id, int spr_id, int quad_idx, int* out_texid);
float* dtex_c2_query_tex(struct dtex_c2*, int key, int* out_texid);
void dtexc2_query_map_addr(struct dtex_c2*, int pkg_id, int spr_id, int quad_idx, struct dtex_texture** out_tex, struct dtex_tp_pos** out_pos);

struct dtex_cg* dtex_c2_get_cg(struct dtex_c2*);

void dtex_c2_debug_draw(struct dtex_c2*);

#endif // dynamic_texture_cache2_h

#ifdef __cplusplus
}
#endif