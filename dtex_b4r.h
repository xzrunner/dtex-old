//#ifdef __cplusplus
//extern "C"
//{
//#endif
//
//#ifndef dynamic_texture_block4_rect_h
//#define dynamic_texture_block4_rect_h
//
//#include <stdint.h>
//
//#ifdef _MSC_VER
//#define EXPORT_RRR
//#endif // _MSC_VER
//
//struct dtex_b4r;
//struct dtex_package;
//
//struct dtex_b4r* dtex_b4r_create(void* data, int sz, int cap);
//void dtex_b4r_release(struct dtex_b4r*);
//
////void dtex_b4r_load_texture(struct dtex_b4r*);
//
//void dtex_b4r_preload_to_pkg(struct dtex_b4r*, struct dtex_package*);
//
//struct dtex_raw_tex* dtex_b4r_load_tex(struct dtex_b4r*, struct dtex_package*, int tex_idx);
//void dtex_b4r_relocate(struct dtex_b4r*, struct dtex_package*);
//
//#ifdef EXPORT_RRR
//size_t dtex_b4r_size(void* data, int sz);
//#endif // EXPORT_RRR
//
//#endif // dynamic_texture_block4_rect_h
//
//#ifdef __cplusplus
//}
//#endif