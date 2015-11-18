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

void dtex_cg_clear(struct dtex_cg*);

void dtex_cg_reload_texture(struct dtex_cg*);

void dtex_cg_load(struct dtex_cg*, uint32_t* buf, int width, int height, struct dtex_glyph* glyph);
void dtex_cg_commit(struct dtex_cg*);

// return texcoords float[8]
float* dtex_cg_query(struct dtex_cg*, struct dtex_glyph* glyph, int* out_texid);

// debug
int dtex_cg_get_texid(struct dtex_cg*);

#endif // dynamic_texture_cache_glyph_h

#ifdef __cplusplus
}
#endif