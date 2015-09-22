//#ifdef __cplusplus
//extern "C"
//{
//#endif
//
//#ifndef dynamic_texture_regular_rect_packer_h
//#define dynamic_texture_regular_rect_packer_h
//
//#include <stdint.h>
//#include <stdbool.h>
//
//#ifdef _MSC_VER
//#define EXPORT_RRP
//#endif // _MSC_VER
//
//struct rrp_rect {
//	int16_t x, y;
//	int16_t w, h;
//};
//
//struct rrp_part {
//	int8_t idx;
//	struct rrp_rect src, dst;
//	bool is_rotated;
//
//	struct dtex_texture* dst_tex;
//	struct dtex_rect* dst_pos;
//};
//
//struct rrp_picture {
//	int16_t id;
//	int16_t w, h;
//
//	int16_t part_sz;
//	struct rrp_part* part;
//};
//
//struct dtex_rrp;
//
//struct dtex_rrp* dtex_rrp_create(void* data, int sz, int cap);
//void dtex_rrp_release(struct dtex_rrp*);
//
//struct rrp_picture* dtex_rrp_get_pic(struct dtex_rrp*, int id);
//
//void dtex_rrp_relocate(struct dtex_rrp* rrp, int idx, struct dtex_texture* tex, struct dtex_rect* pos);
//
//#ifdef EXPORT_RRP
//size_t dtex_rrp_size(void* data, int sz);
//#endif // EXPORT_RRP
//
//#endif // dynamic_texture_regular_rect_packer_h
//
//#ifdef __cplusplus
//}
//#endif