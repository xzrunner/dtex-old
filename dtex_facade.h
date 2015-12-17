#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_facade_h
#define dynamic_texture_facade_h

#include "ejoy2d.h"

#include <stdbool.h>

struct dtex_package;
struct dtex_texture;
struct dtex_tp_pos;

/************************************************************************/
/* dtexf overall                                                        */
/************************************************************************/
void dtexf_create(const char* cfg);
void dtexf_release();

struct dtex_package* dtexf_query_pkg(const char* name);

/************************************************************************/
/* sprite draw                                                          */
/************************************************************************/
void dtexf_sprite_draw(struct dtex_package*, struct ej_sprite*, struct ej_srt*);

/************************************************************************/
/* common load                                                          */
/************************************************************************/
struct dtex_package* dtexf_load_pkg(const char* name, const char* epe_path, float scale, int lod);
void dtexf_unload_pkg(struct dtex_package* pkg);

int dtexf_preload_all_textures(const char* path, struct dtex_package* pkg, float scale);
void dtexf_preload_texture(struct dtex_package* pkg, int idx, float scale);
void dtexf_load_texture(struct dtex_package*, int idx);

// todo sort sprite list for C3
//struct ej_sprite* dtexf_create_sprite(const char* path);

/************************************************************************/
/* C3                                                                   */
/************************************************************************/
void dtexf_c3_load(struct dtex_package* pkg, float scale);
void dtexf_c3_load_end(bool async);
void dtexf_c3_clear();

/************************************************************************/
/* C2                                                                   */
/************************************************************************/
void dtexf_c2_load_begin();
void dtexf_c2_load(struct dtex_package* pkg, int spr_id);
void dtexf_c2_load_tex(int tex_id, int tex_width, int tex_height, int key);
void dtexf_c2_load_end();

void dtexf_c2_remove_tex(int key);

void dtexf_c2_reload_begin();
void dtexf_c2_reload_tex(int tex_id, int tex_width, int tex_height, int key);
void dtexf_c2_reload_end();

float* dtexf_c2_lookup_texcoords(int pkg_id, int spr_id, int* dst_tex);
float* dtexf_c2_query_tex(int key, int* out_texid);
//void dtexf_c2_lookup_node(struct ej_texture* ori_tex, float* ori_vb, struct dtex_texture** out_tex, struct dtex_tp_pos** out_pos);

void dtexf_c2_clear_from_cg();
void dtexf_c2_clear();

/************************************************************************/
/* C1                                                                   */
/************************************************************************/
void dtexf_c1_clear();
void dtexf_c1_bind();
void dtexf_c1_unbind();
uint32_t dtexf_c1_get_texture_id();
uint32_t dtexf_c1_get_texture_size();
//void dtexf_c1_update(struct dtex_package* pkg, struct ej_sprite* spr);
//void dtexf_c1_load_anim(struct dtex_package* pkg, struct animation* ani, int action);
//bool dtexf_c1_draw_anim(struct dtex_package* pkg, struct animation* ani, int action, 
//	int frame, struct draw_params* params);

/************************************************************************/
/* CG                                                                   */
/************************************************************************/
struct dtex_cg* dtexf_get_cg();

/************************************************************************/
/* CS                                                                   */
/************************************************************************/
void dtexf_cs_create();
void dtexf_cs_on_size(int width, int height);
void dtexf_cs_bind();
void dtexf_cs_unbind();
void dtexf_cs_draw_to_screen(void (*before_draw)(void* ud), void* ud);

//void dtexf_async_load_spr(const char* pkg_name, const char* spr_name, const char* path);

/************************************************************************/
/* async load texture                                                   */
/************************************************************************/
void dtexf_async_load_texture(struct dtex_package* pkg, int idx);
bool dtexf_async_load_texture_with_c3(struct dtex_package* pkg, int* spr_ids, int spr_count);
bool dtexf_async_load_texture_with_c2(struct dtex_package* pkg, int* spr_ids, int spr_count);
bool dtexf_async_load_texture_with_c2_from_c3(struct dtex_package* pkg, int* spr_ids, int spr_count);

/************************************************************************/
/* update for async loading                                             */
/************************************************************************/
void dtexf_update();

//
//// RRP
//// todo pkg to rrp
//bool dtexf_draw_rrp(struct ej_package* pkg, struct ej_texture* tex, int id, 
//	struct draw_params* params, const int32_t part_screen[8]);
//
//// PTS
//bool dtexf_draw_pts(struct ej_package* pkg, struct dtex_texture* src, int src_id, 
//	struct dtex_tp_pos* src_pos, struct draw_params* params, const int32_t part_screen[8]);
//
/************************************************************************/
/* debug                                                                */
/************************************************************************/
void dtexf_debug_draw();
//
//// for test
//void dtexf_test_pvr(const char* path);
//void dtexf_test_etc1(const char* path);

#endif // dynamic_texture_facade_h

#ifdef __cplusplus
}
#endif