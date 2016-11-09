#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_cache_glyph_h
#define dynamic_texture_cache_glyph_h

#include <stdint.h>
#include <stdbool.h>

struct dtex_cg;
struct dtex_tp;
struct dtex_texture;
struct dtex_loader;

struct dtex_glyph_style {
	int font;
	int font_size;
	uint32_t font_color;

	bool edge;
	float edge_size;
	uint32_t edge_color;
};

struct dtex_glyph {
	int unicode;
	struct dtex_glyph_style s;
};

struct dtex_cg* dtex_cg_create(struct dtex_tp*, struct dtex_texture*);
void dtex_cg_release(struct dtex_cg*);

void dtex_cg_bitmap_flush(struct dtex_cg*, struct dtex_loader*);

void dtex_cg_clear(struct dtex_cg*);

void dtex_cg_load_bmp(struct dtex_cg*, uint32_t* buf, int width, int height, struct dtex_glyph* glyph);
float* dtex_cg_load_user(struct dtex_cg*, struct dtex_glyph* glyph, float* (*query_and_load_c2)(void* ud, struct dtex_glyph* glyph), void* ud);
void dtex_cg_reload(struct dtex_cg*, uint32_t* buf, int width, int height, float* texcoords);

// return texcoords float[8]
float* dtex_cg_query(struct dtex_cg*, struct dtex_glyph* glyph, int* out_texid);

void dtex_cg_debug_draw(struct dtex_cg*);

#endif // dynamic_texture_cache_glyph_h

#ifdef __cplusplus
}
#endif