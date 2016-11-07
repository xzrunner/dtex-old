#include "dtex_cg.h"
#include "dtex_texture.h"
#include "dtex_gl.h"
#include "dtex_tp.h"
#include "dtex_res_cache.h"
#include "dtex_render.h"
#include "dtex_facade.h"
#include "dtex_shader.h"
#include "dtex_debug.h"

#include <ds_hash.h>
#include <logger.h>

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define BITMAP_EDGE     256
#define MAX_BITMAP_NODE	1024
#define MAX_GLYPH_NODE  1024

#define PADDING 1

struct bitmap_node {
	int w, h;
	struct dtex_tp_pos* pos;
	struct dtex_glyph glyph;
};

struct glyph_node {
	struct dtex_glyph key;
	float texcoords[8];
};

struct dtex_cg {
	// bitmap
	uint32_t*            bitmap;
	struct dtex_tp*      bitmap_tp;
	struct dtex_texture* bitmap_tex;
	struct bitmap_node   bitmap_nodes[MAX_BITMAP_NODE];
	int                  bitmap_node_size;	

	// glyph
	struct dtex_tp*      glyph_tp;
	struct dtex_texture* glyph_tex;
	struct ds_hash*      glyph_hash;
	struct glyph_node    glyph_nodes[MAX_GLYPH_NODE];
	int                  glyph_node_size;

	void (*c2_clear_part)(void* ud);
	void* ud;
};

static inline unsigned int
_hash_func(int hash_sz, void* key) {
	struct dtex_glyph* hk = (struct dtex_glyph*)key;
	uint32_t hash;
	if (hk->s.edge) {
		hash = 
			hk->unicode ^ 
			(hk->s.font * 97) ^ 
			(hk->s.font_size * 101) ^
			hk->s.font_color ^ 
			(int)(hk->s.edge_size * 10000) ^
			hk->s.edge_color;
	} else {
		hash = 
			hk->unicode ^ 
			(hk->s.font * 97) ^ 
			(hk->s.font_size * 101) ^
			hk->s.font_color;
	}
	return hash % hash_sz;
}

static inline bool
_equal_func(void* key0, void* key1) {
	struct dtex_glyph* hk0 = (struct dtex_glyph*)key0;
	struct dtex_glyph* hk1 = (struct dtex_glyph*)key1;
	if (hk0->unicode == hk1->unicode && 
		hk0->s.font == hk1->s.font && 
		hk0->s.font_size	== hk1->s.font_size && 
		hk0->s.font_color == hk1->s.font_color && 
		hk0->s.edge == hk1->s.edge) {
		if (hk0->s.edge) {
			return hk0->s.edge_size	== hk1->s.edge_size
				&& hk0->s.edge_color == hk1->s.edge_color;
		} else {
			return true;
		}
	} else {
		return false;
	}
}

struct dtex_cg* 
dtex_cg_create(struct dtex_tp* tp, struct dtex_texture* tex,
			   void (*c2_clear_part)(void* ud), void* ud) {
	size_t sz = sizeof(struct dtex_cg);
	struct dtex_cg* cg = (struct dtex_cg*)malloc(sz);
	memset(cg, 0, sz);

	cg->bitmap     = (uint32_t*)malloc(BITMAP_EDGE * BITMAP_EDGE * 4);
	cg->bitmap_tp  = dtex_tp_create(BITMAP_EDGE, BITMAP_EDGE, 256);
	cg->bitmap_tex = dtex_texture_create_mid(BITMAP_EDGE, BITMAP_EDGE);

	cg->glyph_tp   = tp;
	cg->glyph_tex  = tex;
	cg->glyph_hash = ds_hash_create(MAX_GLYPH_NODE, MAX_GLYPH_NODE * 2, 0.5f, _hash_func, _equal_func);

	cg->c2_clear_part = c2_clear_part;
	cg->ud = ud;

	return cg;
}

void 
dtex_cg_release(struct dtex_cg* cg) {
	free(cg->bitmap);
	dtex_tp_release(cg->bitmap_tp);
	dtex_res_cache_return_mid_texture(cg->bitmap_tex);

	ds_hash_release(cg->glyph_hash);

	free(cg);
}

static void
_insert_bitmap(struct dtex_cg* cg, struct dtex_loader* loader, struct bitmap_node* bmp) {
	struct dtex_tp_pos* pos = dtex_tp_add(cg->glyph_tp, bmp->w + PADDING * 2, bmp->h + PADDING * 2, false);
	if (!pos) {
		dtexf_c2_clear_from_cg();
		pos = dtex_tp_add(cg->glyph_tp, bmp->w, bmp->h, false);
		assert(pos);
	}

	float vb[16];
	float src_xmin = (bmp->pos->r.xmin + PADDING) * cg->bitmap_tex->inv_width,
		  src_xmax = (bmp->pos->r.xmax - PADDING) * cg->bitmap_tex->inv_width,
		  src_ymin = (bmp->pos->r.ymin + PADDING) * cg->bitmap_tex->inv_height,
		  src_ymax = (bmp->pos->r.ymax - PADDING) * cg->bitmap_tex->inv_height;
	float dst_xmin = (pos->r.xmin + PADDING) * cg->glyph_tex->inv_width * 2 - 1,
		  dst_xmax = (pos->r.xmax - PADDING) * cg->glyph_tex->inv_width * 2 - 1,
		  dst_ymin = (pos->r.ymin + PADDING) * cg->glyph_tex->inv_height * 2 - 1,
		  dst_ymax = (pos->r.ymax - PADDING) * cg->glyph_tex->inv_height * 2 - 1;
	vb[0] = dst_xmin; vb[1] = dst_ymin; vb[2] = src_xmin; vb[3] = src_ymin;
	vb[4] = dst_xmin; vb[5] = dst_ymax; vb[6] = src_xmin; vb[7] = src_ymax;
	vb[8] = dst_xmax; vb[9] = dst_ymax; vb[10]= src_xmax; vb[11]= src_ymax;
	vb[12]= dst_xmax; vb[13]= dst_ymin; vb[14]= src_xmax; vb[15]= src_ymin;
	dtex_draw_to_texture(cg->bitmap_tex, cg->glyph_tex, vb);

	struct glyph_node* node = &cg->glyph_nodes[cg->glyph_node_size++];
	float xmin = (pos->r.xmin + PADDING) * cg->glyph_tex->inv_width,
		  xmax = (pos->r.xmax - PADDING) * cg->glyph_tex->inv_width,
		  ymin = (pos->r.ymin + PADDING) * cg->glyph_tex->inv_height,
		  ymax = (pos->r.ymax - PADDING) * cg->glyph_tex->inv_height;
	node->texcoords[0] = xmin;	node->texcoords[1] = ymin;
	node->texcoords[2] = xmin;	node->texcoords[3] = ymax;
	node->texcoords[4] = xmax;	node->texcoords[5] = ymax;
	node->texcoords[6] = xmax;	node->texcoords[7] = ymin;

	node->key = bmp->glyph;
	ds_hash_insert(cg->glyph_hash, &node->key, node, true);
}

static int
_compare_max_edge(const void* arg1, const void* arg2) {
	struct bitmap_node *node1, *node2;

	node1 = *((struct bitmap_node**)(arg1));
	node2 = *((struct bitmap_node**)(arg2));
 
	int long1, long2, short1, short2;
	if (node1->w > node1->h) {
		long1 = node1->w;
		short1 = node1->h;
	} else {
		long1 = node1->h;
		short1 = node1->w;
	}
	if (node2->w > node2->h) {
		long2 = node2->w;
		short2 = node2->h;
	} else {
		long2 = node2->h;
		short2 = node2->w;
	}

	if (long1 > long2) {
		return -1;
	} else if (long1 < long2) {
		return 1;
	} else {
		if (short1 > short2) return -1;
		else if (short1 < short2) return 1;
		else return 0;
	}
}

void 
dtex_cg_bitmap_flush(struct dtex_cg* cg, struct dtex_loader* loader) {
	if (cg->bitmap_node_size == 0) {
		return;
	}

	dtex_gl_update_texture(cg->bitmap, BITMAP_EDGE, BITMAP_EDGE, cg->bitmap_tex->id);

	struct bitmap_node* sorted[cg->bitmap_node_size];
	for (int i = 0; i < cg->bitmap_node_size; ++i) {
		sorted[i] = &cg->bitmap_nodes[i];
	}
	qsort((void*)sorted, cg->bitmap_node_size, sizeof(struct bitmap_node*), _compare_max_edge);

	dtex_shader_scissor(false);
	dtex_draw_begin();
	for (int i = 0; i < cg->bitmap_node_size; ++i) {
		_insert_bitmap(cg, loader, sorted[i]);
	}
	dtex_draw_end();
	dtex_shader_scissor(true);

	cg->bitmap_node_size = 0;
}

void 
dtex_cg_clear(struct dtex_cg* cg) {
	cg->glyph_node_size = 0;
	ds_hash_clear(cg->glyph_hash);
	cg->c2_clear_part(cg->ud);
}

static bool
_is_glyph_equal(const struct dtex_glyph* g0, const struct dtex_glyph* g1) {
	return g0->unicode      == g1->unicode
		&& g0->s.font       == g1->s.font
		&& g0->s.font_size  == g1->s.font_size
		&& g0->s.font_color == g1->s.font_color
		&& g0->s.edge       == g1->s.edge
		&& g0->s.edge_size  == g1->s.edge_size
		&& g0->s.edge_color == g1->s.edge_color;
}

static void
_bitmap_clear(struct dtex_cg* cg) {
	memset(cg->bitmap, 0, sizeof(BITMAP_EDGE * BITMAP_EDGE * 4));
	dtex_tp_clear(cg->bitmap_tp);
	dtex_texture_clear(cg->bitmap_tex);
	cg->bitmap_node_size = 0;
}

void 
dtex_cg_load_bmp(struct dtex_cg* cg, uint32_t* buf, int width, int height, struct dtex_glyph* glyph) {
	for (int i = 0; i < cg->bitmap_node_size; ++i) {
		if (_is_glyph_equal(glyph, &cg->bitmap_nodes[i].glyph)) {
			return;
		}
	}

	if (cg->glyph_node_size >= MAX_GLYPH_NODE) {
		LOGW("%s", "dtex cg glyph nodes full.");
		return;
	}

	if (cg->bitmap_node_size > MAX_BITMAP_NODE) {	
		_bitmap_clear(cg);
	}
	struct dtex_tp_pos* pos = dtex_tp_add(cg->bitmap_tp, width + PADDING * 2, height + PADDING * 2, false);
	if (!pos) {
		_bitmap_clear(cg);
		pos = dtex_tp_add(cg->bitmap_tp, width + PADDING * 2, height + PADDING * 2, false);
	}
	assert(pos);

	struct bitmap_node* node = &cg->bitmap_nodes[cg->bitmap_node_size++];
	node->w     = width;
	node->h     = height;
	node->pos   = pos;
	pos->ud     = node;
	node->glyph = *glyph;
	int src_ptr = 0;
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			uint32_t src = buf[src_ptr++];
			uint8_t r = (src >> 24) & 0xff;
			uint8_t g = (src >> 16) & 0xff;
			uint8_t b = (src >> 8) & 0xff;
			uint8_t a = src & 0xff;
			int dst_ptr = (pos->r.ymin + PADDING + y) * BITMAP_EDGE + pos->r.xmin + PADDING + x;
			cg->bitmap[dst_ptr] = a << 24 | b << 16 | g << 8 | r;
		}
	}
}

float* 
dtex_cg_load_user(struct dtex_cg* cg, struct dtex_glyph* glyph, float* (*query_and_load_c2)(void* ud, struct dtex_glyph* glyph), void* ud) {
	if (cg->glyph_node_size >= MAX_GLYPH_NODE) {
		LOGW("%s", "cg nodes empty.");
		return NULL;
	}
	
	float* texcoords = query_and_load_c2(ud, glyph);
	if (!texcoords) {
		return NULL;
	}

	struct glyph_node* node = &cg->glyph_nodes[cg->glyph_node_size++];
	memcpy(node->texcoords, texcoords, sizeof(float) * 8);
	node->key = *glyph;
	ds_hash_insert(cg->glyph_hash, &node->key, node, true);

	return node->texcoords;
}

void 
dtex_cg_reload(struct dtex_cg* cg, uint32_t* buf, int width, int height, float* texcoords) {
// 	size_t sz = width * height * sizeof(uint32_t);
// 	assert(sz <= cg->buf_sz);
// 	for (int i = 0, n = width * height; i < n; ++i) {
// 		uint32_t src = buf[i];
// 		uint8_t r = (src >> 24) & 0xff;
// 		uint8_t g = (src >> 16) & 0xff;
// 		uint8_t b = (src >> 8) & 0xff;
// 		uint8_t a = src & 0xff;
// 		cg->buf[i] = a << 24 | b << 16 | g << 8 | r;
// 	}
// 
// 	int x = texcoords[0] * cg->glyph_tex->width,
// 		y = texcoords[5] * cg->glyph_tex->height;
// 	dtex_gl_update_texture(cg->buf, x, y, width, height, cg->glyph_tex->id);
}

float* 
dtex_cg_query(struct dtex_cg* cg, struct dtex_glyph* glyph, int* out_texid) {
	struct glyph_node* node = (struct glyph_node*)ds_hash_query(cg->glyph_hash, glyph);
	if (!node) {
		return NULL;
	}

	*out_texid = cg->glyph_tex->id;

	return node->texcoords;
}

void
dtex_cg_debug_draw(struct dtex_cg* cg) {
	dtex_debug_draw(cg->bitmap_tex->id, 2);
}