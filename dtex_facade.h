#ifndef dynamic_texture_facade_h
#define dynamic_texture_facade_h

#include "dtex_typedef.h"

#include <stdbool.h>

// todo
struct dtex_texture;
struct dp_position;

struct animation;
struct texture;

void dtexf_create(const char* cfg);
void dtexf_release();

// loading
void dtexf_load_pkg(const char* name, const char* path);

// C3
struct ej_package* dtexf_c3_load_pkg(const char* name, const char* path, float scale);
void dtexf_c3_load_pkg_finish();

// C2
void dtexf_c2_load_begin();
void dtexf_c2_load_sprite(struct ej_package* pkg, const char* name);
void dtexf_c2_load_end();
float* dtexf_c2_lookup_texcoords(struct texture* ori_tex, float* ori_vb, int* dst_tex);
void dtexf_c2_lookup_node(struct texture* ori_tex, float* ori_vb, struct dtex_texture** out_tex, struct dp_position** out_pos);

// C1
void dtexf_c1_load_anim(struct ej_package* pkg, struct animation* ani, int action);
bool dtexf_c1_draw_anim(struct ej_package* pkg, struct animation* ani, int action, 
	int frame, struct draw_params* params);

// async loading
void dtexf_async_load_spr(const char* pkg_name, const char* spr_name, const char* path);
void dtexf_update();

// RRP
// todo pkg to rrp
bool dtexf_draw_rrp(struct ej_package* pkg, struct texture* tex, int id, 
	struct draw_params* params, const int32_t part_screen[8]);

// PTS
bool dtexf_draw_pts(struct ej_package* pkg, struct dtex_texture* src, int src_id, 
	struct dp_position* src_pos, struct draw_params* params, const int32_t part_screen[8]);

// for debug
void dtexf_debug_draw();
void dtexf_del_texture(int tex);

// for test
void dtexf_test_pvr(const char* path);

#endif // dynamic_texture_facade_h