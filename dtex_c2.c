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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MAX_TEX_SIZE 128

#define NODE_SIZE 4096*2
#define HASH_SIZE 7561
#define PRELOAD_SIZE 4096*2

#define PADDING 1

struct dtex_node {
	// ori info
	struct dtex_texture* ori_tex;
	struct dtex_rect ori_rect;

	// dst info
	struct dtex_texture* dst_tex;
	struct dp_pos* dst_pos;
	float dst_vb[8];

	// draw ori to dst
	float trans_vb[16];
};

struct hash_node {
	struct hash_node* next_hash;
	struct dtex_node n;
};

struct preload_node {
	struct ej_sprite_pack* ej_pkg;
	struct ej_pack_quad* ej_quad;
	struct dtex_texture* ori_tex;

	struct dtex_rect rect;
};

// todo hash preload_list
struct dtex_c2 {
	int loadable;

	struct dtex_texture* textures[MAX_TEX_SIZE];
	int tex_size;

	struct hash_node* freelist;
	struct hash_node* hash[HASH_SIZE];

	struct preload_node* preload_list[PRELOAD_SIZE];
	int preload_size;
};

void
_reset_preload_list(struct dtex_c2* c2) {
	size_t nsize = NODE_SIZE * sizeof(struct hash_node);
	struct preload_node* first_node = (struct preload_node*)((intptr_t)c2->freelist + nsize);
	for (int i = 0; i < PRELOAD_SIZE; ++i) {
		c2->preload_list[i] = first_node+i;
	}
	c2->preload_size = 0;
}

struct dtex_c2* 
dtex_c2_create(int texture_size) {
	size_t nsize = NODE_SIZE * sizeof(struct hash_node);
	size_t psize = PRELOAD_SIZE * sizeof(struct preload_node);
	size_t sz = sizeof(struct dtex_c2) + nsize + psize;
	struct dtex_c2* c2 = (struct dtex_c2*)malloc(sz);
	memset(c2, 0, sz);

	struct dtex_texture* tex = dtex_res_cache_fetch_mid_texture(texture_size);
	tex->t.MID.packer = dtexpacker_create(tex->width, tex->height, PRELOAD_SIZE);
	c2->textures[c2->tex_size++] = tex;

	c2->freelist = (struct hash_node*)(c2 + 1);
	for (int i = 0; i < NODE_SIZE - 1; ++i) {
		struct hash_node* hn = &c2->freelist[i];
		hn->next_hash = &c2->freelist[i+1];
	}
	c2->freelist[NODE_SIZE-1].next_hash = NULL;

	_reset_preload_list(c2);

	return c2;
}

void 
dtex_c2_release(struct dtex_c2* c2) {
	for (int i = 0; i < c2->tex_size; ++i) {
		dtex_res_cache_return_mid_texture(c2->textures[i]);
	}
	free(c2);
}

void 
dtex_c2_load_begin(struct dtex_c2* c2) {
	c2->loadable++;
}

// todo hash pic for preload_list

struct preload_picture_params {
	struct dtex_c2* c2;
	struct dtex_package* pkg;
};

static inline void
_preload_picture(int pic_id, struct ej_pack_picture* ej_pic, void* ud) {
	struct preload_picture_params* params = (struct preload_picture_params*)ud;

	if (params->c2->loadable == 0 || params->c2->preload_size >= PRELOAD_SIZE - 1) {
		return;
	}

	for (int i = 0; i < ej_pic->n; ++i) {
		if (params->c2->preload_size == PRELOAD_SIZE - 1) {
			break;
		}
		struct ej_pack_quad* ej_q = &ej_pic->rect[i];
		struct preload_node* pn = params->c2->preload_list[params->c2->preload_size++];
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

// todo hash sprite for preload_list
void 
dtex_c2_load(struct dtex_c2* c2, struct dtex_package* pkg, int spr_id) {
	assert(spr_id >= 0);

	if (c2->loadable == 0 || c2->preload_size >= PRELOAD_SIZE - 1) {
		return;
	}

	struct preload_picture_params params;
	params.c2 = c2;
	params.pkg = pkg;
	dtex_ej_spr_traverse(pkg->ej_pkg, spr_id, _preload_picture, &params);
}

static inline int 
_compare_bound(const void *arg1, const void *arg2) {
	struct preload_node *node1, *node2;

	node1 = *((struct preload_node**)(arg1));
	node2 = *((struct preload_node**)(arg2));

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

static inline bool 
_is_rect_same(struct dtex_rect* r0, struct dtex_rect* r1) {
	return r0->xmin == r1->xmin 
		&& r0->ymin == r1->ymin 
		&& r0->xmax == r1->xmax 
		&& r0->ymax == r1->ymax;		
}

static inline void
_unique_nodes(struct dtex_c2* c2) {
	qsort((void*)c2->preload_list, c2->preload_size, sizeof(struct preload_node*), _compare_bound);

	struct preload_node* unique[PRELOAD_SIZE];
	unique[0] = c2->preload_list[0];
	int unique_size = 1;
	for (int i = 1; i < c2->preload_size; ++i) {
		struct preload_node* last = unique[unique_size-1];
		struct preload_node* curr = c2->preload_list[i];
		if ((curr->ori_tex->id == last->ori_tex->id) && 
			_is_rect_same(&curr->rect, &last->rect)) {
			;
		} else {
			unique[unique_size++] = curr;
		}
	}
	memcpy(c2->preload_list, unique, unique_size*sizeof(struct preload_node*));
	c2->preload_size = unique_size;
}

static inline int
_compare_max_edge(const void *arg1, const void *arg2) {
	struct preload_node *node1, *node2;

	node1 = *((struct preload_node**)(arg1));
	node2 = *((struct preload_node**)(arg2));

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

static inline unsigned int
_hash_node(unsigned int texid, struct dtex_rect* rect) {
	int cx = (int)(0.5f * (rect->xmin + rect->xmax)),
		cy = (int)(0.5f * (rect->ymin + rect->ymax));
	return (cx ^ (cy * 97) ^ (texid * 101)) % HASH_SIZE;
}

static inline struct hash_node*
_query_node(struct dtex_c2* c2, unsigned int texid, struct dtex_rect* rect) {
	unsigned int idx = _hash_node(texid, rect);
	struct hash_node* hn = c2->hash[idx];
	while (hn) {
		struct dtex_node* n = &hn->n;
		if (n->ori_tex->id == texid && _is_rect_same(&n->ori_rect, rect)) {
			return hn;
		}
		hn = hn->next_hash;
	}
	return NULL;
}

static inline struct hash_node* 
_new_hash_rect(struct dtex_c2* c2) {
	if (c2->freelist == NULL) {
		return NULL;
	}
	struct hash_node* ret = c2->freelist;
	c2->freelist = ret->next_hash;
	return ret;
}

static inline void
_set_rect_vb(struct preload_node* pn, struct dtex_node* n, bool rotate) {
	struct dtex_inv_size src_sz;
	src_sz.inv_w = pn->ori_tex->inv_width;
	src_sz.inv_h = pn->ori_tex->inv_height;

	struct dtex_inv_size dst_sz;	
	dst_sz.inv_w = n->dst_tex->inv_width;
	dst_sz.inv_h = n->dst_tex->inv_height;

	int rotate_times = rotate ? 1 : 0;
	dtex_relocate_quad(pn->ej_quad->texture_coord, &src_sz, &n->ori_rect, &dst_sz, &n->dst_pos->r, rotate_times, n->trans_vb, n->dst_vb);
}

static inline void
_insert_node(struct dtex_c2* c2, struct dtex_loader* loader, struct preload_node* pn, bool use_new_tex) {
	struct hash_node* hn = _query_node(c2, pn->ori_tex->id, &pn->rect);
	if (hn != NULL) {
		return;
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
			return;
		}
	}
    
    rotate = (pos->is_rotated && !rotate) ||
             (!pos->is_rotated && rotate);

	// save info
	pos->ud = &hn->n;
	hn = _new_hash_rect(c2);
	if (hn == NULL) {
		return;
	}
	assert(tex);
	hn->n.ori_tex = pn->ori_tex;
	hn->n.ori_rect = pn->rect;
	hn->n.dst_tex = tex;
	hn->n.dst_pos = pos;
	hn->n.dst_pos->r.xmin += PADDING;
	hn->n.dst_pos->r.ymin += PADDING;
	hn->n.dst_pos->r.xmax -= PADDING;
	hn->n.dst_pos->r.ymax -= PADDING;

	_set_rect_vb(pn, &hn->n, rotate);

	unsigned int idx = _hash_node(pn->ori_tex->id, &pn->rect);
	hn->next_hash = c2->hash[idx];
	c2->hash[idx] = hn;

	if (rrp_pic) {
//		dtex_draw_rrp_to_tex(hn->n.ori_tex, rrp_pic, tex, pos, rotate);
	} else {
		dtex_draw_to_texture(hn->n.ori_tex, tex, hn->n.trans_vb);
	}
}

void 
dtex_c2_load_end(struct dtex_c2* c2, struct dtex_loader* loader, bool use_only_one_texture) {
	if (--c2->loadable > 0 || c2->preload_size == 0) {
		return;
	}
	_unique_nodes(c2);

	// todo scale
	if (use_only_one_texture) {

	}

	// insert
	dtex_draw_before();
	qsort((void*)c2->preload_list, c2->preload_size, sizeof(struct preload_node*), _compare_max_edge);	
	for (int i = 0; i < c2->preload_size; ++i) {
		_insert_node(c2, loader, c2->preload_list[i], !use_only_one_texture);
	}
	dtex_draw_after();

	_reset_preload_list(c2);
}

static inline void
_get_pic_ori_rect(int ori_w, int ori_h, float* ori_vb, struct dtex_rect* rect) {
	float xmin = 1, ymin = 1, xmax = 0, ymax = 0;
	for (int i = 0; i < 4; ++i) {
		if (ori_vb[i*4+2] < xmin) xmin = ori_vb[i*4+2];
		if (ori_vb[i*4+2] > xmax) xmax = ori_vb[i*4+2];
		if (ori_vb[i*4+3] < ymin) ymin = ori_vb[i*4+3];
		if (ori_vb[i*4+3] > ymax) ymax = ori_vb[i*4+3];
	}
	rect->xmin = ori_w * xmin;
	rect->ymin = ori_h * ymin;
	rect->xmax = ori_w * xmax;
	rect->ymax = ori_h * ymax;
}

float* 
dtex_c2_lookup_texcoords(struct dtex_c2* c2, struct dtex_texture* tex, float vb[16], int* out_texid) {
	struct dtex_rect rect;
	_get_pic_ori_rect(tex->width, tex->height, vb, &rect);

	struct hash_node* hn = _query_node(c2, tex->id, &rect);
	if (hn == NULL) {
		return NULL;
	}

	*out_texid = hn->n.dst_tex->id;
	return hn->n.dst_vb;
}

void 
dtexc2_lookup_node(struct dtex_c2* c2, int texid, struct dtex_rect* rect,
	               struct dtex_texture** out_tex, struct dp_pos** out_pos) {
	struct hash_node* hn = _query_node(c2, texid, rect);
	if (hn == NULL) {
		*out_tex = NULL;
		*out_pos = NULL;
		return;
	}
	*out_tex = hn->n.dst_tex;
	*out_pos = hn->n.dst_pos;
}

void 
dtex_c2_change_key(struct dtex_c2* c2, struct dtex_texture_with_rect* src, struct dtex_texture_with_rect* dst) {    
	unsigned int idx = _hash_node(src->tex->id, &src->rect);
	struct hash_node* last = NULL;
	struct hash_node* curr = c2->hash[idx];
	while (curr) {
		struct dtex_node* n = &curr->n;
		if ((n->ori_tex == src->tex) && _is_rect_same(&n->ori_rect, &src->rect)) {
			break;
		}
		last = curr;

		curr = curr->next_hash;
	}

	if (!curr) {
		return;
	}
	assert(curr);

	curr->n.ori_tex = dst->tex;
	curr->n.ori_rect = dst->rect;

	if (last) {
		last->next_hash = curr->next_hash;
	} else {
		c2->hash[idx] = curr->next_hash;
	}
	unsigned int new_idx = _hash_node(dst->tex->id, &dst->rect);
	curr->next_hash = c2->hash[new_idx];
	c2->hash[new_idx] = curr;
}

void 
dtex_c2_debug_draw(struct dtex_c2* c2) {
	dtex_debug_draw(c2->textures[0]->id);

	// const float edge = 0.5f;
	// int col = 2 / edge;
	// for (int i = 0; i < 16; ++i) {
	// 	int x = i % col;
	// 	int y = i / col;
	// 	dtex_debug_draw_with_pos(i + 1, 
	// 		-1 + x * edge, 1 - y * edge - edge, -1 + x * edge + edge, 1 - y * edge);
	// }
}