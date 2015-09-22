//#ifdef __cplusplus
//extern "C"
//{
//#endif
//
//#ifndef dynamic_texture_regular_rect_raw_h
//#define dynamic_texture_regular_rect_raw_h
//
//#include <stdint.h>
//
//#ifdef _MSC_VER
//#define EXPORT_RRR
//#endif // _MSC_VER
//
//struct dtex_rrr;
//struct dtex_package;
//
//struct dtex_rrr* dtex_rrr_create(void* data, int sz, int cap);
//void dtex_rrr_release(struct dtex_rrr*);
//
////// todo rm
////void dtex_rrr_load_to_c3(struct dtex_rrr*, struct dtex_c3*);
//
//void dtex_rrr_preload_to_pkg(struct dtex_rrr*, struct dtex_package*);
//
//struct dtex_raw_tex* dtex_rrr_load_tex(struct dtex_rrr*, struct dtex_package*, int tex_idx);
//void dtex_rrr_relocate(struct dtex_rrr*, struct dtex_package*);
//
//#ifdef EXPORT_RRR
//size_t dtex_rrr_size(void* data, int sz);
//#endif // EXPORT_RRR
//
//#endif // dynamic_texture_regular_rect_raw_h
//
//#ifdef __cplusplus
//}
//#endif