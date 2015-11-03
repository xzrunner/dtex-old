//#ifdef __cplusplus
//extern "C"
//{
//#endif
//
//#ifndef dynamic_texture_cache_glyph_h
//#define dynamic_texture_cache_glyph_h
//
//#include <stdint.h>
//#include <stdbool.h>
//
//struct dtex_cg;
//
//struct dtex_glyph_style {
//	uint32_t color;
//	int size;
//	int font;
//	bool edge;
//};
//
//struct dtex_glyph {
//	int unicode;
//	struct dtex_glyph_style style;
//};
//
//struct dtex_cg* dtex_cg_create(struct dtex_tp*, struct dtex_texture*, int buf_sz);
//void dtex_cg_release(struct dtex_cg*);
//
//void dtex_cg_clear(struct dtex_cg*);
//
//void dtex_cg_load(struct dtex_cg*, uint8_t* buf, int width, int height, struct dtex_glyph* glyph);
//void dtex_cg_commit(struct dtex_cg*);
//
//// return texcoords float[8]
//float* dtex_cg_query(struct dtex_cg*, int unicode, uint32_t color, int size, int font, bool edge, int* out_texid);
//
//#endif // dynamic_texture_cache_glyph_h
//
//#ifdef __cplusplus
//}
//#endif