#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_facade_h
#define dynamic_texture_facade_h

#include "ejoy2d.h"

#include <stdbool.h>

// todo
struct dtex_texture;
struct dp_pos;

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
struct dtex_package* dtexf_load_pkg(const char* name, const char* path, int type, float scale, int lod, int use_c2);
void dtexf_preload_texture(struct dtex_package* pkg, int idx, float scale);
void dtexf_load_texture(struct dtex_package*, int idx, bool create_by_ej);

// todo sort sprite list for C3
//struct ej_sprite* dtexf_create_sprite(const char* path);

/************************************************************************/
/* C3                                                                   */
/************************************************************************/
void dtexf_c3_load(struct dtex_package* pkg, float scale);
void dtexf_c3_load_end(bool async);

/************************************************************************/
/* C2                                                                   */
/************************************************************************/
void dtexf_c2_load_begin();
void dtexf_c2_load(struct dtex_package* pkg, int spr_id);
void dtexf_c2_load_end();
float* dtexf_c2_lookup_texcoords(struct dtex_texture* ori_tex, float* ori_vb, int* dst_tex);
//void dtexf_c2_lookup_node(struct ej_texture* ori_tex, float* ori_vb, struct dtex_texture** out_tex, struct dp_pos** out_pos);

/************************************************************************/
/* C1                                                                   */
/************************************************************************/
void dtexf_c1_update(struct dtex_package* pkg, struct ej_sprite* spr);
//void dtexf_c1_load_anim(struct dtex_package* pkg, struct animation* ani, int action);
//bool dtexf_c1_draw_anim(struct dtex_package* pkg, struct animation* ani, int action, 
//	int frame, struct draw_params* params);

//void dtexf_async_load_spr(const char* pkg_name, const char* spr_name, const char* path);

/************************************************************************/
/* async load texture                                                   */
/************************************************************************/
void dtexf_async_load_texture(struct dtex_package* pkg, int idx);

/************************************************************************/
/* 1. C3 load small scale 2. async load origin size texture             */
/************************************************************************/
void dtexf_async_load_texture_from_c3(struct dtex_package* pkg, int* sprite_ids, int sprite_count);

/************************************************************************/
/* 1. normal loading 2. async load needed texture and pack to C2        */
/************************************************************************/
void dtexf_async_load_texture_with_c2(struct dtex_package* pkg, int* sprite_ids, int sprite_count);

/************************************************************************/
/* 1. C3 loading 2. async load needed texture and pack to C2            */
/************************************************************************/
void dtexf_async_load_texture_with_c2_from_c3(struct dtex_package* pkg, int* sprite_ids, int sprite_count);

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
//	struct dp_pos* src_pos, struct draw_params* params, const int32_t part_screen[8]);
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