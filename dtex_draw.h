#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_draw_h
#define dynamic_texture_draw_h

#include <stdint.h>
#include <stdbool.h>

// void dtex_draw_rrp(struct dtex_texture* src, struct rrp_picture* pic, 
// 	struct draw_params* params, const int32_t part_screen[8]);
// 
// void dtex_draw_pts(struct dtex_texture* src_tex, struct dp_pos* src_pos, struct pts_picture* src_pts_pic,
// 	struct draw_params* params, const int32_t part_screen[8]);

void dtex_draw_before();
void dtex_draw_after();
// todo 应该是批量的在一个target上画完再切到下一个target
void dtex_draw_to_texture(struct dtex_texture* src, struct dtex_texture* dst, const float vb[16]);
//// todo 一个pic的part可能位于不同的tex中
//void dtex_draw_rrp_to_tex(struct dtex_texture* src, struct rrp_picture* pic, 
//	struct dtex_texture* dst, struct dp_pos* pos, bool rotate);

void dtex_debug_draw(unsigned int texid);
void dtex_debug_draw_with_pos(unsigned int texid, float xmin, float ymin, float xmax, float ymax);

#ifndef USED_IN_EDITOR
void dtex_debug_draw_ej(int uid_3rd, int pos);
#endif // USED_IN_EDITOR

//void dtex_debug_draw(struct dtex_texture*);

void dtex_flush_shader();

#endif // dynamic_texture_draw_h

#ifdef __cplusplus
}
#endif