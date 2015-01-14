#include "dtex_c2.h"
#include "dtex_buffer.h"
#include "dtex_texture.h"
#include "dtex_packer.h"
#include "dtex_draw.h"
#include "dtex_loader.h"
#include "dtex_utility.h"
#include "dtex_rrp.h"

#include "package.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MAX_TEX_SIZE 128

#define NODE_SIZE 4096*2
#define HASH_SIZE 7561
#define PRELOAD_SIZE 4096*2

struct dtex_node {
	// ori info
	struct dtex_raw_tex ori_tex;
	struct dtex_rect ori_rect;

	// dst info
	struct dtex_texture* dst_tex;
	struct dp_position* dst_pos;
	float dst_vb[8];

	// draw ori to dst
	float trans_vb[16];
};

struct hash_node {
	struct hash_node* next_hash;
	struct dtex_node n;
};

struct preload_node {
	struct ej_package* pkg;
	struct picture_part* part;
	struct texture tex;

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

struct dtex_c2* 
dtexc2_create(struct dtex_buffer* buf) {
	size_t nsize = NODE_SIZE * sizeof(struct hash_node);
	size_t psize = PRELOAD_SIZE * sizeof(struct preload_node);
	size_t sz = sizeof(struct dtex_c2) + nsize + psize;
	struct dtex_c2* dtex = (struct dtex_c2*)malloc(sz);
	memset(dtex, 0, sz);

	dtexbuf_reserve(buf, 1);
	dtex->textures[dtex->tex_size++] = dtex_new_tex_with_packer(buf, PRELOAD_SIZE);

	dtex->freelist = (struct hash_node*)(dtex + 1);
	for (int i = 0; i < NODE_SIZE - 1; ++i) {
		struct hash_node* hn = &dtex->freelist[i];
		hn->next_hash = &dtex->freelist[i+1];
	}
	dtex->freelist[NODE_SIZE-1].next_hash = NULL;

	struct preload_node* first_node = (struct preload_node*)((intptr_t)dtex->freelist + nsize);
	for (int i = 0; i < PRELOAD_SIZE; ++i) {
		dtex->preload_list[i] = first_node+i;
	}

	return dtex;
}

void 
dtexc2_release(struct dtex_c2* dtex, struct dtex_buffer* buf) {
	for (int i = 0; i < dtex->tex_size; ++i) {
		dtex_del_tex(buf, dtex->textures[i]);
	}

	free(dtex);
}

void 
dtexc2_preload_begin(struct dtex_c2* dtex) {
	dtex->loadable++;
}

// todo hash pic for preload_list
static inline void
_preload_picture(struct dtex_c2* dtex, struct ej_package* pkg, struct picture* pic) {
	if (dtex->loadable == 0 || dtex->preload_size >= PRELOAD_SIZE - 1) {
		return;
	}

	assert(pic->n < 0);
	for (int i = 0; i < -pic->n; ++i) {
		if (dtex->preload_size == PRELOAD_SIZE - 1) {
			break;
		}
		struct picture_part* part = &pic->part[i];
		struct preload_node* pn = dtex->preload_list[dtex->preload_size++];
		pn->pkg = pkg;
		pn->part = part;
		assert(pkg->texture_n > part->texid);
		pn->tex = pkg->tex[part->texid];
		dtex_get_pic_src_rect(part->src, &pn->rect);
	}
}

// todo hash anim for preload_list
static inline void
_preload_animation(struct dtex_c2* dtex, struct ej_package* pkg, int id) {
	if (dtex->loadable == 0 || dtex->preload_size >= PRELOAD_SIZE - 1) {
		return;
	}

	struct ejoypic* ep = pkg->ep;
	if (id < 0 || ep->max_id < id) {
		return;
	}
	struct animation* ani = ep->spr[id];
	if (ani == NULL) {
		return;
	}

	if (ani->part_n <= 0) {
		_preload_picture(dtex, pkg, (struct picture*)ani);
	} else {
		for (int i = 0; i < ani->part_n; ++i) {
			struct animation_part* part = &ani->part[i];
			if (!part->text) {
				_preload_animation(dtex, pkg, part->id);
			}
		}
	}
}

// todo hash sprite for preload_list
void 
dtexc2_preload_sprite(struct dtex_c2* dtex, struct ej_package* pkg, int id) {
	assert(id >= 0);
	_preload_animation(dtex, pkg, id);
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

	if(node1->tex.id < node2->tex.id) return -1;
	if(node1->tex.id > node2->tex.id) return 1;

	return 0;
}

static inline void
_unique_preload_list(struct dtex_c2* dtex) {
	qsort((void*)dtex->preload_list, dtex->preload_size, sizeof(struct preload_node*), _compare_bound);
	struct preload_node* unique[PRELOAD_SIZE];
	unique[0] = dtex->preload_list[0];
	int unique_size = 1;
	for (int i = 1; i < dtex->preload_size; ++i) {
		struct preload_node* last = unique[unique_size-1];
		struct preload_node* curr = dtex->preload_list[i];
		if (curr->tex.id == last->tex.id && dtex_rect_same(&curr->rect, &last->rect)) {
			;
		} else {
			unique[unique_size++] = curr;
		}
	}
	memcpy(dtex->preload_list, unique, unique_size*sizeof(struct preload_node*));
	dtex->preload_size = unique_size;
}

static inline int
_compare_edge(const void *arg1, const void *arg2) {
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
_hash_node(GLuint texid, struct dtex_rect* rect) {
	int cx = (int)(0.5f * (rect->xmin + rect->xmax)),
		cy = (int)(0.5f * (rect->ymin + rect->ymax));
	return (cx ^ (cy * 97) ^ (texid * 101)) % HASH_SIZE;
}

static inline struct hash_node*
_query_node(struct dtex_c2* dtex, GLuint texid, struct dtex_rect* rect) {
	unsigned int idx = _hash_node(texid, rect);
	struct hash_node* hn = dtex->hash[idx];
	while (hn) {
		struct dtex_node* n = &hn->n;
		if (n->ori_tex.id == texid && dtex_rect_same(&n->ori_rect, rect)) {
			return hn;
		}
		hn = hn->next_hash;
	}
	return NULL;
}

static inline struct hash_node* 
_new_hash_rect(struct dtex_c2* dtex) {
	if (dtex->freelist == NULL) {
		return NULL;
	}
	struct hash_node* ret = dtex->freelist;
	dtex->freelist = ret->next_hash;
	return ret;
}

static inline void
_set_rect_vb(struct preload_node* pn, struct dtex_node* n, bool rotate) {
	struct dtex_inv_size src_sz;
	src_sz.inv_w = pn->tex.width;
	src_sz.inv_h = pn->tex.height;

	struct dtex_inv_size dst_sz;	
	dst_sz.inv_w = 1.0f / n->dst_tex->width;
	dst_sz.inv_h = 1.0f / n->dst_tex->height;

	dtex_relocate_pic_part(pn->part->src, &src_sz, &n->ori_rect, &dst_sz, &n->dst_pos->r, rotate, n->trans_vb, n->dst_vb);
}

static inline void
_insert_node(struct dtex_c2* dtex, struct dtex_buffer* buf, struct dtex_loader* loader, struct preload_node* pn, bool use_new_tex) {
	struct hash_node* hn = _query_node(dtex, pn->tex.id, &pn->rect);
	if (hn != NULL) {
		return;
	}

	// insert to packer
	int w = pn->rect.xmax - pn->rect.xmin,
		h = pn->rect.ymax - pn->rect.ymin;
	// rrp
	struct dr_picture* rrp_pic = NULL;
	if (pn->rect.xmin < 0) {
		struct dtex_rrp* rrp = dtexloader_query_rrp(loader, pn->pkg);
		if (rrp == NULL) {
			return;
		}
		rrp_pic = dtex_rrp_get_pic(rrp, -pn->rect.xmin);
		assert(rrp_pic);
		w = rrp_pic->w;
		h = rrp_pic->h;
	}
	struct dp_position* pos = NULL;
	struct dtex_texture* tex = NULL;
	bool rotate = false;
	for (int i = 0; i < dtex->tex_size && pos == NULL; ++i) {
		tex = dtex->textures[i];
		// todo padding and rotate
	//	if (w >= h) {
			pos = dtexpacker_add(tex->packer, w, h);
			rotate = false;
	//	} else {
	//		pos = dtexpacker_add(tex->packer, h, w);
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
	hn = _new_hash_rect(dtex);
	if (hn == NULL) {
		return;
	}
	assert(tex);
	hn->n.ori_tex.id = pn->tex.id;
	hn->n.ori_tex.id_alpha = pn->tex.id_alpha;
	hn->n.ori_tex.width = 1 / pn->tex.width;
	hn->n.ori_tex.height = 1 / pn->tex.height;
	hn->n.ori_tex.format = pn->tex.format;
	hn->n.ori_rect = pn->rect;
	hn->n.dst_tex = tex;
	hn->n.dst_pos = pos;
	_set_rect_vb(pn, &hn->n, rotate);

	unsigned int idx = _hash_node(pn->tex.id, &pn->rect);
	hn->next_hash = dtex->hash[idx];
	dtex->hash[idx] = hn;

	if (rrp_pic) {
		dtex_draw_rrp_to_tex(buf, &hn->n.ori_tex, rrp_pic, tex, pos);
	} else {
		dtex_draw_to_texture(buf, &hn->n.ori_tex, hn->n.trans_vb, tex);
	}
}

void 
dtexc2_preload_end(struct dtex_c2* dtex, struct dtex_buffer* buf, struct dtex_loader* loader, bool use_only_one_texture) {
	if (--dtex->loadable > 0 || dtex->preload_size == 0) {
		return;
	}
	_unique_preload_list(dtex);

	// todo scale
	if (use_only_one_texture) {

	}

	// insert
	qsort((void*)dtex->preload_list, dtex->preload_size, sizeof(struct preload_node*), _compare_edge);	
	for (int i = 0; i < dtex->preload_size; ++i) {
		_insert_node(dtex, buf, loader, dtex->preload_list[i], !use_only_one_texture);
	}

	dtex->preload_size = 0;
}

float* 
dtexc2_lookup_texcoords(struct dtex_c2* dtex, int texid, struct dtex_rect* rect, int* out_texid) {
	struct hash_node* hn = _query_node(dtex, texid, rect);
	if (hn == NULL) {
		return NULL;
	}

	*out_texid = hn->n.dst_tex->tex;
	return hn->n.dst_vb;
}

void 
dtexc2_lookup_node(struct dtex_c2* dtex, int texid, struct dtex_rect* rect,
	struct dtex_texture** out_tex, struct dp_position** out_pos) {

	struct hash_node* hn = _query_node(dtex, texid, rect);
	if (hn == NULL) {
		*out_tex = NULL;
		*out_pos = NULL;
		return;
	}
	*out_tex = hn->n.dst_tex;
	*out_pos = hn->n.dst_pos;
}

void 
dtexc2_change_key(struct dtex_c2* dtex, int src_texid, struct dtex_rect* src_rect, int dst_texid, struct dtex_rect* dst_rect) {    
	unsigned int idx = _hash_node(src_texid, src_rect);
	struct hash_node* last = NULL;
	struct hash_node* curr = dtex->hash[idx];
	while (curr) {
		struct dtex_node* n = &curr->n;
		if (n->ori_tex.id == src_texid && dtex_rect_same(&n->ori_rect, src_rect)) {
			break;
		}
		last = curr;
		curr = curr->next_hash;
	}

	// if (!curr) {
	// 	return;
	// }
	assert(curr);
	curr->n.ori_tex.id = dst_texid;
	curr->n.ori_rect = *dst_rect;

	if (last) {
		last->next_hash = curr->next_hash;
	} else {
		dtex->hash[idx] = curr->next_hash;
	}
	unsigned int new_idx = _hash_node(dst_texid, dst_rect);
	curr->next_hash = dtex->hash[new_idx];
	dtex->hash[new_idx] = curr;
}

void 
dtexc2_debug_draw(struct dtex_c2* dtex) {
	dtex_debug_draw(dtex->textures[0]->tex);
}