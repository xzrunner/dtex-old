#include "dtex_c2.h"
#include "dtex_tp.h"
#include "dtex_loader.h"
#include "dtex_rrp.h"
#include "dtex_package.h"
#include "dtex_ej_utility.h"
#include "dtex_relocation.h"
#include "dtex_texture.h"
#include "dtex_res_cache.h"
#include "dtex_hash.h"
#include "dtex_log.h"
#include "dtex_c2_strategy.h"
#include "dtex_debug.h"
#include "dtex_render.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MAX_TEX_SIZE 4

#define NODE_SIZE 4096*2
#define PRELOAD_SIZE 4096*2

#define PADDING 1
//#define EXTRUDE 1

struct hash_key {
	unsigned int texid;
	struct dtex_rect rect;
};

struct c2_node {
	// ori info
	struct dtex_texture* ori_tex;
	struct dtex_rect ori_rect;

	// dst info
	struct dtex_texture* dst_tex;
	struct dtex_tp_pos* dst_pos;
	float dst_vb[8];

	// draw ori to dst
	float trans_vb[16];

	// hash
	struct hash_key hk;
};

struct c2_prenode {
	struct dtex_package* pkg;
	struct ej_pack_quad* ej_quad;
	struct dtex_texture* ori_tex;

	struct dtex_rect rect;
};

struct tp_index {
	struct c2_node nodes[NODE_SIZE];
	int node_size;

	struct dtex_hash* hash;

	struct dtex_tp* tp;

	bool is_static;
};

// todo hash prenodes
struct dtex_c2 {
	int loadable;

	bool one_tex_mode;
	union {
		struct {
			struct dtex_texture* texture;
			struct tp_index index[4];	// 0 1
										// 2 3
			int clear_idx;
		} ONE;

		struct {
 			struct dtex_texture* textures[MAX_TEX_SIZE];
 			int tex_size;
			struct tp_index index;
		} MULTI;
	} t;

	int prenode_size;	
	struct c2_prenode prenodes[1];
};

static inline unsigned int 
_hash_func(int hash_sz, void* key) {
	struct hash_key* hk = (struct hash_key*)key;
	int cx = (int)(0.5f * (hk->rect.xmin + hk->rect.xmax)),
		cy = (int)(0.5f * (hk->rect.ymin + hk->rect.ymax));
	return (cx ^ (cy * 97) ^ (hk->texid * 101)) % hash_sz;
}

static inline bool 
_equal_func(void* key0, void* key1) {
	struct hash_key* hk0 = (struct hash_key*)key0;
	struct hash_key* hk1 = (struct hash_key*)key1;
	return hk0->rect.xmin == hk1->rect.xmin 
		&& hk0->rect.ymin == hk1->rect.ymin 
		&& hk0->rect.xmax == hk1->rect.xmax 
		&& hk0->rect.ymax == hk1->rect.ymax
		&& hk0->texid == hk1->texid;
}

struct dtex_c2* 
dtex_c2_create(int texture_size, bool one_tex_mode, int static_count) {
	size_t sz = sizeof(struct dtex_c2) + sizeof(struct c2_prenode) * PRELOAD_SIZE;
	struct dtex_c2* c2 = (struct dtex_c2*)malloc(sz);
	if (!c2) {
		return NULL;
	}
	memset(c2, 0, sz);

	c2->one_tex_mode = one_tex_mode;
	if (one_tex_mode) {
		c2->t.ONE.texture = dtex_res_cache_fetch_mid_texture(texture_size);
		int half_sz = texture_size >> 1;
		for (int i = 0; i < 4; ++i) {
			struct tp_index* index = &c2->t.ONE.index[i];
			index->hash = dtex_hash_create(1000, 2000, 0.5f, _hash_func, _equal_func);
			index->tp = dtex_tp_create(half_sz, half_sz, PRELOAD_SIZE / 4);
			index->is_static = i < static_count;
		}
	} else {
		struct dtex_texture* tex = dtex_res_cache_fetch_mid_texture(texture_size);
		tex->t.MID.tp = dtex_tp_create(tex->width, tex->height, PRELOAD_SIZE);
		c2->t.MULTI.textures[c2->t.MULTI.tex_size++] = tex;

		c2->t.MULTI.index.hash = dtex_hash_create(1000, 2000, 0.5f, _hash_func, _equal_func);
		c2->t.MULTI.index.tp = NULL;
	}

	c2->prenode_size = 0;

	return c2;
}

void 
dtex_c2_release(struct dtex_c2* c2) {
	if (c2->one_tex_mode) {
		dtex_res_cache_return_mid_texture(c2->t.ONE.texture);
		for (int i = 0; i < 4; ++i) {
			dtex_hash_release(c2->t.ONE.index[i].hash);
			dtex_tp_release(c2->t.ONE.index[i].tp);
		}
	} else {
		for (int i = 0; i < c2->t.MULTI.tex_size; ++i) {
			dtex_res_cache_return_mid_texture(c2->t.MULTI.textures[i]);
		}
		dtex_hash_release(c2->t.MULTI.index.hash);
	}

	free(c2);
}

static inline void
_clear_tp_index(struct tp_index* index) {
	index->node_size = 0;

	dtex_hash_clear(index->hash);

	if (index->tp) {
		dtex_tp_clear(index->tp);
	}
}

static inline void
dtex_c2_clear(struct dtex_c2* c2, struct dtex_loader* loader) {
	dtex_warning(" c2 clear");

	c2->loadable = 0;

	if (c2->one_tex_mode) {
		int idx = c2->t.ONE.clear_idx;

		float x = 0, y = 0;
		if (idx == 0 || idx == 1) {
			y = 0.5f;
		}
		if (idx == 3 || idx == 1) {
			x = 0.5f;
		}

		dtex_texture_clear_part(c2->t.ONE.texture, x, y, x+0.5f, y+0.5f);
		_clear_tp_index(&c2->t.ONE.index[idx]);
		c2->t.ONE.clear_idx = (idx + 1) % 4;
	} else {
		for (int i = 0; i < c2->t.MULTI.tex_size; ++i) {
			struct dtex_texture* tex = c2->t.MULTI.textures[i];
			assert(tex->type == DTEX_TT_MID);
			dtex_texture_clear(tex);
			dtex_tp_clear(tex->t.MID.tp);
		}
		_clear_tp_index(&c2->t.MULTI.index);
	}

	c2->prenode_size = 0;

	dtex_package_traverse(loader, dtex_c2_strategy_clear);
}

void 
dtex_c2_load_begin(struct dtex_c2* c2) {
	c2->loadable++;
}

struct preload_picture_params {
	struct dtex_c2* c2;
	struct dtex_package* pkg;
};

static inline void
_preload_picture(int pic_id, struct ej_pack_picture* ej_pic, void* ud) {
	struct preload_picture_params* params = (struct preload_picture_params*)ud;

	if (params->c2->loadable == 0) {
		return;
	}

	if (params->c2->prenode_size >= PRELOAD_SIZE - 1) {
		dtex_warning("c2 prenode full.");
		return;
	}

	for (int i = 0; i < ej_pic->n; ++i) {
		if (params->c2->prenode_size == PRELOAD_SIZE - 1) {
			break;
		}
		struct ej_pack_quad* ej_q = &ej_pic->rect[i];
		struct c2_prenode* pn = &params->c2->prenodes[params->c2->prenode_size++];
		pn->pkg = params->pkg;
		pn->ej_quad = ej_q;
		if (ej_q->texid < QUAD_TEXID_IN_PKG_MAX) {
			pn->ori_tex = params->pkg->textures[ej_q->texid];
		} else {
			pn->ori_tex = dtex_texture_fetch(ej_q->texid);
		}
		dtex_get_texcoords_region(ej_q->texture_coord, &pn->rect);
	}
}

void 
dtex_c2_load(struct dtex_c2* c2, struct dtex_package* pkg, int spr_id) {
	assert(spr_id >= 0);

	if (c2->loadable == 0) {
		return;
	}

	if (c2->prenode_size >= PRELOAD_SIZE - 1) {
		dtex_warning("c2 prenode full.");
		return;
	}

	struct preload_picture_params params;
	params.c2 = c2;
	params.pkg = pkg;
	dtex_ej_spr_traverse(pkg->ej_pkg, spr_id, _preload_picture, &params);
}

static inline int 
_compare_bound(const void *arg1, const void *arg2) {
	struct c2_prenode *node1, *node2;

	node1 = *((struct c2_prenode**)(arg1));
	node2 = *((struct c2_prenode**)(arg2));

	if(node1->rect.xmin < node2->rect.xmin) return -1;
	if(node1->rect.xmin > node2->rect.xmin) return 1;

	if(node1->rect.xmax < node2->rect.xmax) return -1;
	if(node1->rect.xmax > node2->rect.xmax) return 1;

	if(node1->rect.ymin < node2->rect.ymin) return -1;
	if(node1->rect.ymin > node2->rect.ymin) return 1;

	if(node1->rect.ymax < node2->rect.ymax) return -1;
	if(node1->rect.ymax > node2->rect.ymax) return 1;

	if(node1->ori_tex->id < node2->ori_tex->id) return -1;
	if(node1->ori_tex->id > node2->ori_tex->id) return 1;

	return 0;
}

static inline void
_get_unique_prenodes(struct dtex_c2* c2, struct c2_prenode** ret_set, int* ret_sz) {
	for (int i = 0; i < c2->prenode_size; ++i) {
		ret_set[i] = &c2->prenodes[i];
	}
	qsort((void*)ret_set, c2->prenode_size, sizeof(struct c2_prenode*), _compare_bound);

	struct c2_prenode* unique[PRELOAD_SIZE];
	unique[0] = ret_set[0];
	int unique_size = 1;
	for (int i = 1; i < c2->prenode_size; ++i) {
		struct c2_prenode* last = unique[unique_size-1];
		struct c2_prenode* curr = ret_set[i];
		if (_compare_bound(&curr, &last) == 0) {
			;
		} else {
			unique[unique_size++] = curr;
		}
	}
	memcpy(ret_set, unique, sizeof(struct c2_prenode*) * unique_size);
	*ret_sz = unique_size;
}

static inline int
_compare_max_edge(const void *arg1, const void *arg2) {
	struct c2_prenode *node1, *node2;

	node1 = *((struct c2_prenode**)(arg1));
	node2 = *((struct c2_prenode**)(arg2));

	int16_t w1 = node1->rect.xmax - node1->rect.xmin,
			h1 = node1->rect.ymax - node1->rect.ymin;
	int16_t w2 = node2->rect.xmax - node2->rect.xmin,
			h2 = node2->rect.ymax - node2->rect.ymin;
	int16_t long1, long2, short1, short2;
	if (w1 > h1) {
		long1 = w1;
		short1 = h1;
	} else {
		long1 = h1;
		short1 = w1;
	}
	if (w2 > h2) {
		long2 = w2;
		short2 = h2;
	} else {
		long2 = h2;
		short2 = w2;
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

static inline struct c2_node*
_query_node(struct dtex_c2* c2, unsigned int texid, struct dtex_rect* rect) {
	struct hash_key hk;
	hk.texid = texid;
	hk.rect = *rect;

	if (c2->one_tex_mode) {
		for (int i = 0; i < 4; ++i) {
			struct c2_node* ret = (struct c2_node*)dtex_hash_query(c2->t.ONE.index[i].hash, &hk);
			if (ret) {
				return ret;
			}
		}
	} else {
		return (struct c2_node*)dtex_hash_query(c2->t.MULTI.index.hash, &hk);
	}
	return NULL;
}

static inline void
_set_rect_vb(struct c2_prenode* pn, struct c2_node* n, bool rotate) {
	struct dtex_inv_size src_sz;
	src_sz.inv_w = pn->ori_tex->inv_width;
	src_sz.inv_h = pn->ori_tex->inv_height;

	struct dtex_inv_size dst_sz;	
	dst_sz.inv_w = n->dst_tex->inv_width;
	dst_sz.inv_h = n->dst_tex->inv_height;

	int rotate_times = rotate ? 1 : 0;
	dtex_relocate_draw_vb(pn->ej_quad->texture_coord, &src_sz, &n->ori_rect, &dst_sz, &n->dst_pos->r, rotate_times, n->trans_vb);
	dtex_relocate_c2_val(pn->ej_quad->texture_coord, &src_sz, &n->ori_rect, &dst_sz, &n->dst_pos->r, rotate_times, n->dst_vb);
}

struct insert_params {
	struct dtex_c2* c2;
	struct dtex_loader* loader;
	int w;
	int h;
	bool can_clear;

	bool rotate;
	struct dtex_tp_pos* pos;
	struct dtex_texture* tex;
	struct tp_index* index;
};

static inline bool
_mode_one_insert_node(struct insert_params* p) {
	p->tex = p->c2->t.ONE.texture;
	assert(p->tex->type == DTEX_TT_MID);
	float half_edge = p->c2->t.ONE.texture->width * 0.5f;
	
	for (int i = 0; i < 4; ++i) {
		struct tp_index* index = &p->c2->t.ONE.index[i];
		if (index->is_static == p->can_clear) {
			continue;
		}
		p->pos = dtex_tp_add(index->tp, p->w + PADDING * 2, p->h + PADDING * 2, true);
		p->rotate = false;
		if (p->pos) {
			p->index = index;
			if (i == 0 || i == 1) {
				p->pos->r.ymin += half_edge;
				p->pos->r.ymax += half_edge;
			}
			if (i == 3 || i == 1) {
				p->pos->r.xmin += half_edge;
				p->pos->r.xmax += half_edge;
			}
			return true;
		}
	}

	if (p->can_clear) {
		// todo
		// 1. use new texture
		// 2. scale
		// 3. clear
		dtex_c2_clear(p->c2, p->loader);
	}

	return false;
}

static inline bool
_mode_multi_insert_node(struct insert_params* p) {
	for (int i = 0; i < p->c2->t.MULTI.tex_size && p->pos == NULL; ++i) {
		p->tex = p->c2->t.MULTI.textures[i];
		assert(p->tex->type == DTEX_TT_MID);
		// todo padding and rotate
		//	if (w >= h) {
		p->pos = dtex_tp_add(p->tex->t.MID.tp, p->w + PADDING * 2, p->h + PADDING * 2, true);
		p->rotate = false;
		//	} else {
		//		pos = dtex_tp_add(tex->tp, h, w, true);
		//		rotate = true;
		//	}
	}

	if (p->pos) {
		p->index = &p->c2->t.MULTI.index;
	} else {
		// todo
		// 1. use new texture
		// 2. scale
		// 3. clear
		dtex_c2_clear(p->c2, p->loader);
		return false;
	}
	return true;
}

static inline bool
_insert_node(struct dtex_c2* c2, struct dtex_loader* loader, struct c2_prenode* pn) {
	if (_query_node(c2, pn->ori_tex->id, &pn->rect)) {
		return true;
	}

	struct insert_params ip;
	ip.c2 = c2;
	ip.loader = loader;
	ip.w = pn->rect.xmax - pn->rect.xmin,
	ip.h = pn->rect.ymax - pn->rect.ymin;
	ip.can_clear = dtex_c2_insert_can_clear(pn->pkg->c2_stg);

	// rrp
	struct rrp_picture* rrp_pic = NULL;
	if (pn->rect.xmin < 0) {
// 		struct dtex_rrp* rrp = dtexloader_query_rrp(loader, pn->ej_pkg);
// 		if (rrp == NULL) {
// 			return;
// 		}
// 		rrp_pic = dtex_rrp_get_pic(rrp, -pn->rect.xmin);
// 		assert(rrp_pic);
// 		w = rrp_pic->w;
// 		h = rrp_pic->h;
	}

	ip.rotate = false;
	ip.pos = NULL;
	ip.tex = NULL;
	ip.index = NULL;
	bool inserted;
	if (c2->one_tex_mode) {
		inserted = _mode_one_insert_node(&ip);
	} else {
		inserted = _mode_multi_insert_node(&ip);
	}
	if (!inserted) {
		return false;
	}
    
    ip.rotate = (ip.pos->is_rotated && !ip.rotate) ||
		        (!ip.pos->is_rotated && ip.rotate);

	// save info

	struct c2_node* node = NULL;
	if (ip.index->node_size == NODE_SIZE) {
		dtex_warning(" c2 nodes empty.");
		return false;
	}
	node = &ip.index->nodes[ip.index->node_size++];

	assert(ip.tex);
	node->ori_tex = pn->ori_tex;
	node->ori_rect = pn->rect;
	node->dst_tex = ip.tex;
	node->dst_pos = ip.pos;
	node->dst_pos->r.xmin += PADDING;
	node->dst_pos->r.ymin += PADDING;
	node->dst_pos->r.xmax -= PADDING;
	node->dst_pos->r.ymax -= PADDING;

	node->hk.texid = pn->ori_tex->id;
	node->hk.rect = pn->rect;
	dtex_hash_insert(ip.index->hash, &node->hk, node, true);

	_set_rect_vb(pn, node, ip.rotate);

	ip.pos->ud = node;

	if (rrp_pic) {
//		dtex_draw_rrp_to_tex(node->ori_tex, rrp_pic, tex, pos, rotate);
	} else {
		dtex_draw_to_texture(node->ori_tex, ip.tex, node->trans_vb);
	}

	return true;
}

void 
dtex_c2_load_end(struct dtex_c2* c2, struct dtex_loader* loader) {
	if (--c2->loadable > 0 || c2->prenode_size == 0) {
		return;
	}

	struct c2_prenode* unique_set[c2->prenode_size];
	int unique_sz = 0;
	_get_unique_prenodes(c2, unique_set, &unique_sz);

	// insert
	qsort((void*)unique_set, unique_sz, sizeof(struct c2_prenode*), _compare_max_edge);	
	for (int i = 0; i < unique_sz; ++i) {
		bool succ = _insert_node(c2, loader, unique_set[i]);
		if (!succ) {
			break;
		}
	}
	dtex_draw_finish();

	c2->prenode_size = 0;
}

static inline void
_get_pic_ori_rect(int ori_w, int ori_h, float* ori_vb, struct dtex_rect* rect) {
	float xmin = 1, ymin = 1, xmax = 0, ymax = 0;
	for (int i = 0; i < 4; ++i) {
		if (ori_vb[i*2] < xmin) xmin = ori_vb[i*2];
		if (ori_vb[i*2] > xmax) xmax = ori_vb[i*2];
		if (ori_vb[i*2+1] < ymin) ymin = ori_vb[i*2+1];
		if (ori_vb[i*2+1] > ymax) ymax = ori_vb[i*2+1];
	}
	rect->xmin = ori_w * xmin;
	rect->ymin = ori_h * ymin;
	rect->xmax = ori_w * xmax;
	rect->ymax = ori_h * ymax;
}

float* 
dtex_c2_lookup_texcoords(struct dtex_c2* c2, struct dtex_texture* tex, float vb[8], int* out_texid) {
	struct dtex_rect rect;
	_get_pic_ori_rect(tex->width, tex->height, vb, &rect);

	struct c2_node* node = _query_node(c2, tex->id, &rect);
	if (!node) {
		return NULL;
	}

	*out_texid = node->dst_tex->id;
	return node->dst_vb;
}

void 
dtexc2_lookup_node(struct dtex_c2* c2, int texid, struct dtex_rect* rect,
	               struct dtex_texture** out_tex, struct dtex_tp_pos** out_pos) {
	struct c2_node* node = _query_node(c2, texid, rect);
	if (node) {
		*out_tex = node->dst_tex;
		*out_pos = node->dst_pos;
	} else {
		*out_tex = NULL;
		*out_pos = NULL;
	}
}

void 
dtex_c2_change_key(struct dtex_c2* c2, struct dtex_texture_with_rect* src, struct dtex_texture_with_rect* dst) {    
	struct hash_key old_hk;
	old_hk.texid = src->tex->id;
	old_hk.rect = src->rect;

	struct c2_node* node = NULL;
	struct dtex_hash* hash = NULL;
	if (c2->one_tex_mode) {
		for (int i = 0; i < 4; ++i) {
			hash = c2->t.ONE.index[i].hash;
			node = (struct c2_node*)dtex_hash_remove(hash, &old_hk);
		}
	} else {
		hash = c2->t.MULTI.index.hash;
		node = (struct c2_node*)dtex_hash_remove(hash, &old_hk);
	}

	if (!node) {
		return;
	}

	node->ori_tex = dst->tex;
	node->ori_rect = dst->rect;
	
	node->hk.texid = dst->tex->id;
	node->hk.rect = dst->rect;
	dtex_hash_insert(hash, &node->hk, node, true);
}

void 
dtex_c2_debug_draw(struct dtex_c2* c2) {
#ifdef USED_IN_EDITOR
	dtex_debug_draw(c2->textures[0]->id);
#else
	if (c2->one_tex_mode) {
		dtex_debug_draw_ej(c2->t.ONE.texture->uid_3rd, 1);
	} else {
		if (c2->t.MULTI.tex_size > 0) {
			dtex_debug_draw_ej(c2->t.MULTI.textures[0]->uid_3rd, 1);
			if (c2->t.MULTI.tex_size > 1) {
				dtex_debug_draw_ej(c2->t.MULTI.textures[1]->uid_3rd, 4);
			}
		}
	}
#endif // USED_IN_EDITOR

	// const float edge = 0.5f;
	// int col = 2 / edge;
	// for (int i = 0; i < 16; ++i) {
	// 	int x = i % col;
	// 	int y = i / col;
	// 	dtex_debug_draw_with_pos(i + 1, 
	// 		-1 + x * edge, 1 - y * edge - edge, -1 + x * edge + edge, 1 - y * edge);
	// }
}