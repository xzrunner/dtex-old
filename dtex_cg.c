#include "dtex_cg.h"
#include "dtex_hash.h"
#include "dtex_texture.h"
#include "dtex_gl.h"
#include "dtex_log.h"
#include "dtex_tp.h"

#include "dtex_facade.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MAX_NODE 1024
#define MAX_PRENODE 1024

#define PADDING 1

struct glyph_node {
	struct dtex_glyph key;
	float texcoords[8];
};

struct dtex_cg {
	struct dtex_tp* tp;
	struct dtex_texture* tex;

// 	struct glyph_prenode prenodes[MAX_PRENODE];
// 	int prenode_size;	

	struct glyph_node nodes[MAX_NODE];
	int node_size;

	struct dtex_hash* hash;

	int buf_sz;
	uint32_t* buf;
//	struct dtex_tp* buf_tp;
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
dtex_cg_create(struct dtex_tp* tp, struct dtex_texture* tex) {
	size_t sz = sizeof(struct dtex_cg);
	struct dtex_cg* cg = (struct dtex_cg*)malloc(sz);
	memset(cg, 0, sz);

	cg->tp = tp;
	cg->tex = tex;

	cg->hash = dtex_hash_create(MAX_NODE, MAX_NODE * 2, 0.5f, _hash_func, _equal_func);

	cg->buf = NULL;
	cg->buf_sz = 0;

//	cg->buf_tp = dtex_tp_create(buf_sz, buf_sz, buf_sz * buf_sz / );

	return cg;
}

void 
dtex_cg_release(struct dtex_cg* cg) {
	free(cg);
}

void 
dtex_cg_clear(struct dtex_cg* cg) {
	cg->node_size = 0;
}

void 
dtex_cg_reload_texture(struct dtex_cg* cg) {
	dtex_texture_reload(cg->tex);
}

void 
dtex_cg_load(struct dtex_cg* cg, uint32_t* buf, int width, int height, struct dtex_glyph* glyph) {
	if (cg->node_size >= MAX_NODE) {
		dtex_warning(" cg nodes empty.");
		return;
	}

	struct glyph_node* node = &cg->nodes[cg->node_size++];

	// insert
	//	bool rot = false;
	struct dtex_tp_pos* pos = dtex_tp_add(cg->tp, width + PADDING * 2, height + PADDING * 2, false);
	if (!pos) {
		dtexf_c2_clear_from_cg();
		return;
	}
	float xmin = (pos->r.xmin + PADDING) * cg->tex->inv_width,
		  xmax = (pos->r.xmax - PADDING) * cg->tex->inv_width,
		  ymin = (pos->r.ymin + PADDING) * cg->tex->inv_height,
		  ymax = (pos->r.ymax - PADDING) * cg->tex->inv_height;
	node->texcoords[0] = xmin;	node->texcoords[1] = ymax;
	node->texcoords[4] = xmax;	node->texcoords[5] = ymin;
	node->texcoords[2] = xmin;	node->texcoords[3] = ymin;
	node->texcoords[6] = xmax;	node->texcoords[7] = ymax;
	node->key = *glyph;
	dtex_hash_insert(cg->hash, &node->key, node, true);

	// draw
	size_t sz = width * height * sizeof(uint32_t);
	if (sz > cg->buf_sz) {
		free(cg->buf);
		cg->buf = malloc(sz);
		cg->buf_sz = sz;
	}
	for (int i = 0, n = width * height; i < n; ++i) {
		uint32_t src = buf[i];
		uint8_t r = (src >> 24) & 0xff;
		uint8_t g = (src >> 16) & 0xff;
		uint8_t b = (src >> 8) & 0xff;
		uint8_t a = src & 0xff;
		cg->buf[i] = a << 24 | b << 16 | g << 8 | r;
	}
	dtex_gl_update_texture(cg->buf, pos->r.xmin + PADDING, pos->r.ymin + PADDING, width, height, cg->tex->id);

	return node->texcoords;
}

void 
dtex_cg_reload(struct dtex_cg* cg, uint32_t* buf, int width, int height, float* texcoords) {
	size_t sz = width * height * sizeof(uint32_t);
	assert(sz <= cg->buf_sz);
	for (int i = 0, n = width * height; i < n; ++i) {
		uint32_t src = buf[i];
		uint8_t r = (src >> 24) & 0xff;
		uint8_t g = (src >> 16) & 0xff;
		uint8_t b = (src >> 8) & 0xff;
		uint8_t a = src & 0xff;
		cg->buf[i] = a << 24 | b << 16 | g << 8 | r;
	}

	int x = texcoords[0] * cg->tex->width,
		y = texcoords[5] * cg->tex->height;
	dtex_gl_update_texture(cg->buf, x, y, width, height, cg->tex->id);
}

void 
dtex_cg_commit(struct dtex_cg* cg) {
	
}

float* 
dtex_cg_query(struct dtex_cg* cg, struct dtex_glyph* glyph, int* out_texid) {
	struct glyph_node* node = (struct glyph_node*)dtex_hash_query(cg->hash, glyph);
	if (!node) {
		return NULL;
	}

	*out_texid = cg->tex->id;

	return node->texcoords;
}

int 
dtex_cg_get_texid(struct dtex_cg* cg) {
	return cg->tex->id;
}