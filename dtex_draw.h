//#ifdef __cplusplus
//extern "C"
//{
//#endif
//
//#ifndef dynamic_texture_draw_h
//#define dynamic_texture_draw_h
//
//#include <stdint.h>
//#include <stdbool.h>
//
//struct dtex_raw_tex;
//struct dtex_texture;
//struct dtex_buffer;
//struct draw_params;
//struct rrp_picture;
//struct dp_pos;
//struct pts_picture;
//
//// void dtex_draw_rrp(struct dtex_raw_tex* src, struct rrp_picture* pic, 
//// 	struct draw_params* params, const int32_t part_screen[8]);
//// 
//// void dtex_draw_pts(struct dtex_texture* src_tex, struct dp_pos* src_pos, struct pts_picture* src_pts_pic,
//// 	struct draw_params* params, const int32_t part_screen[8]);
//
//// todo 应该是批量的在一个fbo上画完再切到下一个fbo
//void dtex_draw_to_texture(struct dtex_buffer*, struct dtex_raw_tex* src, const float vb[16], struct dtex_texture* dst);
////// todo 一个pic的part可能位于不同的tex中
////void dtex_draw_rrp_to_tex(struct dtex_buffer*, struct dtex_raw_tex* src, struct rrp_picture* pic, 
////	struct dtex_texture* dst, struct dp_pos* pos, bool rotate);
//
//void dtex_debug_draw(unsigned int texid);
//void dtex_debug_draw_with_pos(unsigned int texid, float xmin, float ymin, float xmax, float ymax);
//
////void dtex_debug_draw(struct dtex_texture*);
//
//void dtex_flush_shader();
//
//#endif // dynamic_texture_draw_h
//
//#ifdef __cplusplus
//}
//#endif