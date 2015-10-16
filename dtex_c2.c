#include "dtex_c2.h"
#include "dtex_packer.h"
#include "dtex_draw.h"
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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MAX_TEX_SIZE 128

#define NODE_SIZE 4096*2
#define PRELOAD_SIZE 4096*2

#define PADDING 1

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
	struct dp_pos* dst_pos;
	float dst_vb[8];

	// draw ori to dst
	float trans_vb[16];

	// hash
	struct hash_key hk;
};

struct c2_prenode {
	struct ej_sprite_pack* ej_pkg;
	struct ej_pack_quad* ej_quad;
	struct dtex_texture* ori_tex;

	struct dtex_rect rect;
};

// todo hash prenodes
struct dtex_c2 {
	int loadable;

	struct dtex_texture* textures[MAX_TEX_SIZE];
	int tex_size;

	struct c2_node nodes[NODE_SIZE];
	int node_size;

	struct dtex_hash* hash;

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
dtex_c2_create(int texture_size) {
	size_t sz = sizeof(struct dtex_c2) + sizeof(struct c2_prenode) * PRELOAD_SIZE;
	struct dtex_c2* c2 = (struct dtex_c2*)malloc(sz);
	if (!c2) {
		return NULL;
	}
	memset(c2, 0, sz);

	struct dtex_texture* tex = dtex_res_cache_fetch_mid_texture(texture_size);
	tex->t.MID.packer = dtexpacker_create(tex->width, tex->height, PRELOAD_SIZE);
	c2->textures[c2->tex_size++] = tex;

	c2->hash = dtex_hash_create(1000, 2000, 0.5f, _hash_func, _equal_func);

	c2->prenode_size = 0;

	return c2;
}

void 
dtex_c2_release(struct dtex_c2* c2) {
	for (int i = 0; i < c2->tex_size; ++i) {
		dtex_res_cache_return_mid_texture(c2->textures[i]);
	}
	dtex_hash_release(c2->hash);
	free(c2);
}

static inline void
dtex_c2_clear(struct dtex_c2* c2, struct dtex_loader* loader) {
	dtex_info("c2 clear");

	c2->loadable = 0;

	for (int i = 0; i < c2->tex_size; ++i) {
		struct dtex_texture* tex = c2->textures[i];
		dtex_texture_clear(tex);
		assert(tex->type == DTEX_TT_MID);
		dtex_tp_clear(tex->t.MID.packer);
	}

	c2->node_size = 0;

	dtex_hash_clear(c2->hash);

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
		pn->ej_pkg = params->pkg->ej_pkg;
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
	return (struct c2_node*)dtex_hash_query(c2->hash, &hk);
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
	dtex_relocate_quad(pn->ej_quad->texture_coord, &src_sz, &n->ori_rect, &dst_sz, &n->dst_pos->r, rotate_times, n->trans_vb, n->dst_vb);
}

static inline bool
_insert_node(struct dtex_c2* c2, struct dtex_loader* loader, struct c2_prenode* pn, bool use_new_tex) {
	if (_query_node(c2, pn->ori_tex->id, &pn->rect)) {
		return true;
	}

	// insert to packer
	int w = pn->rect.xmax - pn->rect.xmin,
		h = pn->rect.ymax - pn->rect.ymin;
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
	struct dp_pos* pos = NULL;
	struct dtex_texture* tex = NULL;
	bool rotate = false;
	for (int i = 0; i < c2->tex_size && pos == NULL; ++i) {
		tex = c2->textures[i];
		assert(tex->type == DTEX_TT_MID);
		// todo padding and rotate
	//	if (w >= h) {
			pos = dtexpacker_add(tex->t.MID.packer, w + PADDING * 2, h + PADDING * 2, true);
			rotate = false;
	//	} else {
	//		pos = dtexpacker_add(tex->packer, h, w, true);
	//		rotate = true;
	//	}
	}

	if (pos == NULL) {
		if (use_new_tex) {
			// todo
		} else {
			dtex_c2_clear(c2, loader);
		}
		return false;
	}
    
    rotate = (pos->is_rotated && !rotate) ||
             (!pos->is_rotated && rotate);

	// save info

	struct c2_node* node = NULL;
	if (c2->node_size == NODE_SIZE) {
		dtex_warning(" c2 nodes empty.");
		return false;
	}
	node = &c2->nodes[c2->node_size++];

	assert(tex);
	node->ori_tex = pn->ori_tex;
	node->ori_rect = pn->rect;
	node->dst_tex = tex;
	node->dst_pos = pos;
	node->dst_pos->r.xmin += PADDING;
	node->dst_pos->r.ymin += PADDING;
	node->dst_pos->r.xmax -= PADDING;
	node->dst_pos->r.ymax -= PADDING;

	node->hk.texid = pn->ori_tex->id;
	node->hk.rect = pn->rect;
	dtex_hash_insert(c2->hash, &node->hk, node, true);

	_set_rect_vb(pn, node, rotate);

	pos->ud = node;

	if (rrp_pic) {
//		dtex_draw_rrp_to_tex(node->ori_tex, rrp_pic, tex, pos, rotate);
	} else {
		dtex_draw_to_texture(node->ori_tex, tex, node->trans_vb);
	}

	return true;
}

void 
dtex_c2_load_end(struct dtex_c2* c2, struct dtex_loader* loader, bool use_only_one_texture) {
	if (--c2->loadable > 0 || c2->prenode_size == 0) {
		return;
	}

	struct c2_prenode* unique_set[c2->prenode_size];
	int unique_sz = 0;
	_get_unique_prenodes(c2, unique_set, &unique_sz);

	// todo scale
	if (use_only_one_texture) {

	}

	// insert
	dtex_draw_before();
	qsort((void*)unique_set, unique_sz, sizeof(struct c2_prenode*), _compare_max_edge);	
	for (int i = 0; i < unique_sz; ++i) {
		bool succ = _insert_node(c2, loader, unique_set[i], !use_only_one_texture);
		if (!succ) {
			break;
		}
	}
	dtex_draw_after();

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
	               struct dtex_texture** out_tex, struct dp_pos** out_pos) {
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
	struct c2_node* node = (struct c2_node*)dtex_hash_remove(c2->hash, &old_hk);
	if (!node) {
		return;
	}

	node->ori_tex = dst->tex;
	node->ori_rect = dst->rect;
	
	node->hk.texid = dst->tex->id;
	node->hk.rect = dst->rect;
	dtex_hash_insert(c2->hash, &node->hk, node, true);
}

void 
dtex_c2_debug_draw(struct dtex_c2* c2) {
#ifdef USED_IN_EDITOR
	dtex_debug_draw(c2->textures[0]->id);
#else
	if (c2->tex_size > 0) {
		dtex_debug_draw_ej(c2->textures[0]->uid_3rd, 1);
		if (c2->tex_size > 1) {
			dtex_debug_draw_ej(c2->textures[1]->uid_3rd, 4);
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