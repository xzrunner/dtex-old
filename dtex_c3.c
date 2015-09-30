#include "dtex_c3.h"
#include "dtex_packer.h"
#include "dtex_texture.h"
#include "dtex_draw.h"
#include "dtex_buffer.h"
#include "dtex_rrp.h"
#include "dtex_rrr.h"
#include "dtex_b4r.h"
#include "dtex_file.h"
#include "dtex_math.h"
#include "dtex_texture_pool.h"
#include "dtex_loader_new.h"
#include "dtex_package.h"
#include "dtex_ej_utility.h"
#include "dtex_texture_loader.h"
#include "dtex_async_loader.h"

#include <ejoy2d.h>

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MAX_TEX_SIZE 128

#define NODE_SIZE 512
#define HASH_SIZE 1021
#define PRELOAD_SIZE 512

#define SCALE_EVERTIME 0.9f
#define MIN_SCALE 0.1f

#define TOT_AREA_SCALE 1.0f

struct dtex_node {
	// ori info
	struct dtex_package* pkg;
	int raw_tex_idx;
	// dest info
	struct dtex_texture* dst_tex;
	struct dtex_rect dst_rect;
	bool is_rotated;
};

struct hash_node {
	struct hash_node* next_hash;
	struct dtex_node n;
};

struct preload_node {
	struct dtex_package* pkg;
	int raw_tex_idx;

	float scale;
};

struct dtex_c3 {
	struct dtex_texture* textures[MAX_TEX_SIZE];
	int tex_size;

	struct hash_node* freelist;
	struct hash_node* hash[HASH_SIZE];

	struct preload_node* preload_list[PRELOAD_SIZE];
	int preload_size;
};

struct dtex_c3* 
dtex_c3_create() {
	size_t nsize = NODE_SIZE * sizeof(struct hash_node);
	size_t psize = PRELOAD_SIZE * sizeof(struct preload_node);
	size_t sz = sizeof(struct dtex_c3) + nsize + psize;
	struct dtex_c3* c3 = (struct dtex_c3*)malloc(sz);
	memset(c3, 0, sz);

	c3->freelist = (struct hash_node*)(c3 + 1);
	for (int i = 0; i < NODE_SIZE - 1; ++i) {
		struct hash_node* hn = &c3->freelist[i];
		hn->next_hash = &c3->freelist[i+1];
	}
	c3->freelist[NODE_SIZE-1].next_hash = NULL;

	struct preload_node* first_node = (struct preload_node*)((intptr_t)c3->freelist + nsize);
	for (int i = 0; i < PRELOAD_SIZE; ++i) {
		c3->preload_list[i] = first_node+i;
	}

	return c3;
}

void dtex_c3_release(struct dtex_c3* c3, struct dtex_buffer* buf) {
	for (int i = 0; i < c3->tex_size; ++i) {
		dtex_del_tex(buf, c3->textures[i]);
	}

	free(c3);
}

void 
dtex_c3_load(struct dtex_c3* c3, struct dtex_package* pkg, float scale) {
	for (int i = 0; i < c3->preload_size; ++i) {
		if (pkg == c3->preload_list[i]->pkg) {
			return;
		}
	}

	for (int i = 0; i < pkg->tex_size; ++i) {
		assert(c3->preload_size <= MAX_TEX_SIZE);
		struct preload_node* n = c3->preload_list[c3->preload_size++];
		n->pkg = pkg;
		n->raw_tex_idx = i;
		n->scale = scale;
	}
}

static inline int
_compare_preload_name(const void *arg1, const void *arg2) {
	struct preload_node *node1, *node2;

	node1 = *((struct preload_node**)(arg1));
	node2 = *((struct preload_node**)(arg2));

	int cmp = strcmp(node1->pkg->name, node2->pkg->name);
	if (cmp == 0) {
		return node1->raw_tex_idx < node2->raw_tex_idx;
	} else {
		return cmp;
	}
}

static inline int
_compare_preload_length(const void *arg1, const void *arg2) {
	struct preload_node *node1, *node2;

	node1 = *((struct preload_node**)(arg1));
	node2 = *((struct preload_node**)(arg2));

	int w1 = node1->pkg->textures[node1->raw_tex_idx]->width,
		h1 = node1->pkg->textures[node1->raw_tex_idx]->height;
	int w2 = node2->pkg->textures[node2->raw_tex_idx]->width,
		h2 = node2->pkg->textures[node2->raw_tex_idx]->height;	

	int16_t long1, long2, short1, short2;
	if (w1 > h1) {
		long1 = w1 * node1->scale;
		short1 = h1 * node1->scale;
	} else {
		long1 = h1 * node1->scale;
		short1 = w1 * node1->scale;
	}
	if (w2 > h2) {
		long2 = w2 * node2->scale;
		short2 = h2 * node2->scale;
	} else {
		long2 = h2 * node2->scale;
		short2 = w2 * node2->scale;
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

static inline void
_unique_nodes(struct dtex_c3* c3) {
	qsort((void*)c3->preload_list, c3->preload_size, sizeof(struct preload_node*), _compare_preload_name);
	struct preload_node* unique[PRELOAD_SIZE];
	unique[0] = c3->preload_list[0];
	int unique_size = 1;
	for (int i = 1; i < c3->preload_size; ++i) {
		struct preload_node* last = unique[unique_size-1];
		struct preload_node* curr = c3->preload_list[i];
		if (strcmp(curr->pkg->name, last->pkg->name) == 0 && curr->raw_tex_idx == last->raw_tex_idx) {
			;
		} else {
			unique[unique_size] = curr;
			++unique_size;
		}
	}
	memcpy(c3->preload_list, unique, unique_size*sizeof(struct preload_node*));
	c3->preload_size = unique_size;	
}

static inline struct hash_node* 
_new_hash_rect(struct dtex_c3* c3) {
	if (c3->freelist == NULL) {
		return NULL;  	
	}
	struct hash_node* ret = c3->freelist;
	c3->freelist = ret->next_hash;
	assert(ret != NULL);
	return ret;
}

// todo use name and id to hash
static inline unsigned int
_hash_origin_pack(const char* name) {
	// BKDR Hash Function
    unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
    unsigned int hash = 0;

    while (*name) {
        hash = hash * seed + (*name++);
    }

    return (hash & 0x7FFFFFFF) % HASH_SIZE;
}

static inline bool
_pack_preload_node(struct dtex_c3* c3, float scale, struct preload_node* node, struct dtex_texture* texture) {
	int w = node->pkg->textures[node->raw_tex_idx]->width * node->scale * scale,
		h = node->pkg->textures[node->raw_tex_idx]->height * node->scale * scale;
	struct dp_pos* pos = NULL;
	// todo padding
	if (w >= h) {
		pos = dtexpacker_add(texture->packer, w, h, true);
	} else {
		pos = dtexpacker_add(texture->packer, h, w, true);
	}
	if (!pos) {
		return false;
	}

	struct hash_node* hn = _new_hash_rect(c3);
	hn->n.pkg = node->pkg;
	hn->n.raw_tex_idx = node->raw_tex_idx;
	hn->n.dst_tex = texture;
	hn->n.dst_rect = pos->r;
	if ((pos->is_rotated && w >= h) ||
		(!pos->is_rotated && h >= w)) {
		hn->n.is_rotated = true;
	}
	pos->ud = &hn->n;

	unsigned int idx = _hash_origin_pack(node->pkg->name);
	hn->next_hash = c3->hash[idx];
	c3->hash[idx] = hn;	

	return true;
}

static inline bool
_pack_preload_list_with_scale(struct dtex_c3* c3, float scale) {
	// init rect packer
	for (int i = 0; i < c3->tex_size; ++i) {
		struct dtex_texture* tex = c3->textures[i];
		if (tex) {
			dtexpacker_release(tex->packer);
		}
		// packer's capacity should larger for later inserting
		tex->packer = dtexpacker_create(tex->width, tex->height, c3->preload_size + 100);
	}
	// insert
	int tex_idx = 0;
	for (int i = 0; i < c3->preload_size; ++i) {
		struct preload_node* node = c3->preload_list[i];
		bool success = false;
		for (int j = tex_idx; j < tex_idx + c3->tex_size && !success; ++j) {
			struct dtex_texture* tex = c3->textures[j % c3->tex_size];
			success = _pack_preload_node(c3, scale, node, tex);
			tex_idx = j;
		}
		if (!success) {
			return false;
		}
	}
	return true;
}

static inline float
_pack_nodes(struct dtex_c3* c3, float alloc_scale) {
	qsort((void*)c3->preload_list, c3->preload_size, 
		sizeof(struct _pack_nodes*), _compare_preload_length);

	float scale = alloc_scale;
	while (scale > MIN_SCALE) {
		bool success = _pack_preload_list_with_scale(c3, scale);
		if (!success) {
			scale *= SCALE_EVERTIME;
        } else {
            break;
        }
	}
	return scale;
}

static inline int
_compare_dr_ori_pkg(const void *arg1, const void *arg2) {
	struct dtex_node *node1, *node2;

	node1 = *((struct dtex_node**)(arg1));
	node2 = *((struct dtex_node**)(arg2));

	return node1->pkg < node2->pkg;
}

// static inline int
// _compare_dr_dst_tex(const void *arg1, const void *arg2) {
// 	struct dtex_node *node1, *node2;
// 
// 	node1 = *((struct dtex_node**)(arg1));
// 	node2 = *((struct dtex_node**)(arg2));
// 
// 	return node1->dst_tex < node2->dst_tex;
// }

struct relocate_pic_data {
	struct dtex_package* pkg;
	int src_tex_idx;
	int dst_tex_idx;
	struct dtex_rect* dst_rect;
};

static inline void
_relocate_pic(struct ej_pack_picture* ej_pic, void* ud) {
	struct dtex_node* dr = (struct dtex_node*)ud;
	for (int i = 0; i < ej_pic->n; ++i) {
		struct pack_quad* ej_q = &ej_pic->rect[i];
		if (ej_q->texid >= QUAD_TEXID_IN_PKG_MAX ||
			ej_q->texid != dr->raw_tex_idx) {
			continue;
		}
		struct dtex_raw_tex* src = dr->pkg->textures[ej_q->texid];
		ej_q->texid = dr->dst_tex->raw_tex->idx + QUAD_TEXID_IN_PKG_MAX;
		for (int j = 0; j < 4; ++j) {
			float x = (float)ej_q->texture_coord[j*2] / src->width;
			float y = (float)ej_q->texture_coord[j*2+1] / src->height;
			ej_q->texture_coord[j*2] = dr->dst_rect.xmin + (dr->dst_rect.xmax - dr->dst_rect.xmin) * x;
			ej_q->texture_coord[j*2+1] = dr->dst_rect.ymin + (dr->dst_rect.ymax - dr->dst_rect.ymin) * y;
		}
	}
}

// todo other format: rrr, b4r

struct relocate_nodes_params {
	struct dtex_buffer* buf;
	struct dtex_node* node;
};

static inline void
_relocate_node(struct dtex_buffer* buf, struct dtex_raw_tex* src, struct dtex_node* dst) {
	// draw old tex to new 
	float tx_min = 0, tx_max = 1,
		ty_min = 0, ty_max = 1;
	float vx_min = (float)dst->dst_rect.xmin / dst->dst_tex->width * 2 - 1,
		vx_max = (float)dst->dst_rect.xmax / dst->dst_tex->width * 2 - 1,
		vy_min = (float)dst->dst_rect.ymin / dst->dst_tex->height * 2 - 1,
		vy_max = (float)dst->dst_rect.ymax / dst->dst_tex->height * 2 - 1;
	float vb[16];
	vb[0] = vx_min; vb[1] = vy_min; vb[2] = tx_min; vb[3] = ty_min;
	vb[4] = vx_min; vb[5] = vy_max; vb[6] = tx_min; vb[7] = ty_max;
	vb[8] = vx_max; vb[9] = vy_max; vb[10] = tx_max; vb[11] = ty_max;
	vb[12] = vx_max; vb[13] = vy_min; vb[14] = tx_max; vb[15] = ty_min;
	dtex_draw_to_texture(buf, src, dst->dst_tex, vb);

	dtex_ej_pkg_traverse(dst->pkg->ej_pkg, _relocate_pic, dst);

	// todo: relocate rrr, b4r
	// 	if (pkg->rrr_pkg) {
	// 		dtex_rrr_relocate(pkg->rrr_pkg, pkg);
	// 	} else if (pkg->b4r_pkg) {
	// 		dtex_b4r_relocate(pkg->b4r_pkg, pkg);
	// 	}
	// 	if (pkg->rrp_pkg) {
	// 		_relocate_rrp(c3, pkg);
	// 	}

}

static inline void
_relocate_nodes_cb(struct dtex_import_stream* is, void* ud) {
	// todo: check file type: rrr, b4r
	struct relocate_nodes_params* params = (struct relocate_nodes_params*)ud;	
	struct dtex_raw_tex* texture = params->node->pkg->textures[params->node->raw_tex_idx];

	bool tex_loaded = false;
	if (texture->id == 0) {
		tex_loaded = false;
		dtex_load_texture_all(params->buf, is, texture);
	} else {
		tex_loaded = true;
	}
	_relocate_node(params->buf, texture, params->node);
	if (!tex_loaded) {
		dtex_pool_remove(texture);
		params->node->pkg->textures[params->node->raw_tex_idx] = NULL;
	}

	free(params);
}

static inline void
_relocate_nodes(struct dtex_c3* c3, struct dtex_loader* loader, struct dtex_buffer* buf, bool async) {
	// sort all node by its texture
	int count = 0;
	struct dtex_node* nodes[NODE_SIZE];
	for (int i = 0; i < HASH_SIZE; ++i) {
		struct hash_node* hn = c3->hash[i];
		while (hn) {
			nodes[count++] = &hn->n;
			hn = hn->next_hash;
		}
	}
	qsort((void*)nodes, count, sizeof(struct dtex_node*), _compare_dr_ori_pkg);

	// draw
	struct dtex_package* last_pkg = NULL;
	for (int i = 0; i < count; ++i) {
		struct dtex_node* dr = nodes[i];
		// change package should flush shader, as texture maybe removed
		if (last_pkg != NULL && dr->pkg != last_pkg) {
			dtex_flush_shader();
		}

		// load old tex

		bool tex_loaded = false;
		struct dtex_raw_tex* ori_tex = NULL;
		if (dr->pkg->rrr_pkg) {
//			ori_tex = dtex_rrr_load_tex(dr->pkg->rrr_pkg, dr->pkg, dr->raw_tex_idx);
		} else if (dr->pkg->b4r_pkg) {
//			ori_tex = dtex_b4r_load_tex(dr->pkg->b4r_pkg, dr->pkg, dr->raw_tex_idx);
		} else {
			ori_tex = dr->pkg->textures[dr->raw_tex_idx];
			if (!async) {
				if (ori_tex->id == 0) {
					tex_loaded = false;
					dtex_load_texture(loader, buf, dr->pkg, dr->raw_tex_idx, ori_tex->scale);
				} else {
					tex_loaded = true;
				}
			} else {
 				struct relocate_nodes_params* params = (struct relocate_nodes_params*)malloc(sizeof(*params));
 				params->buf = buf;
 				params->node = dr;
 				dtex_async_load_file(dr->pkg->tex_filepaths[dr->raw_tex_idx], _relocate_nodes_cb, params);
 			}
		}

 		if (!async) {
 			_relocate_node(buf, ori_tex, dr);
 			if (!tex_loaded) {
 				dtex_pool_remove(ori_tex);
 				dr->pkg->textures[dr->raw_tex_idx] = NULL;
 			}
 		}

		last_pkg = dr->pkg;
	}
}

static inline float
_alloc_texture(struct dtex_c3* c3, struct dtex_buffer* buf) {
	float area = 0;
	for (int i = 0; i < c3->preload_size; ++i) {
		struct preload_node* n = c3->preload_list[i];
		int w = n->pkg->textures[n->raw_tex_idx]->width * n->scale,
			h = n->pkg->textures[n->raw_tex_idx]->height * n->scale;
		area += w * h;
	}
	area *= TOT_AREA_SCALE;	

	int gain = dtexbuf_reserve(buf, area);
	int real = 0;
	do {
		struct dtex_texture* tex = dtex_new_tex(buf);
		real += tex->width * tex->height;

		struct dtex_raw_tex* rtex = dtex_pool_add();
		rtex->id = tex->tex;
		rtex->width = tex->width;
		rtex->height = tex->height;
		rtex->format = TEXTURE8;
		tex->raw_tex = rtex;

		c3->textures[c3->tex_size++] = tex;
	} while (real < gain);

	return MIN(1.0f, (float)gain / area);
}

void 
dtex_c3_load_end(struct dtex_c3* c3, struct dtex_loader* loader, struct dtex_buffer* buf, bool async) {
	if (c3->preload_size == 0) {
		return;
	}

	_unique_nodes(c3);

	float alloc_scale = _alloc_texture(c3, buf);

	/*float scale = */_pack_nodes(c3, alloc_scale);

	_relocate_nodes(c3, loader, buf, async);

    c3->preload_size = 0;
}

struct dp_pos* 
dtex_c3_load_tex(struct dtex_c3* c3, struct dtex_raw_tex* tex, struct dtex_buffer* buf, struct dtex_texture** dst) {
	// todo sort

	// todo select dst texture
	struct dtex_texture* dst_tex = c3->textures[0];
	*dst = dst_tex;

	// insert
	struct dp_pos* pos = dtexpacker_add(dst_tex->packer, tex->width, tex->height, true);
	if (!pos) {
		return NULL;
	}

	// draw
	// draw old tex to new 
	float tx_min = 0, tx_max = 1,
		  ty_min = 0, ty_max = 1;
	float vx_min = (float)pos->r.xmin / dst_tex->width * 2 - 1,
		  vx_max = (float)pos->r.xmax / dst_tex->width * 2 - 1,
		  vy_min = (float)pos->r.ymin / dst_tex->height * 2 - 1,
		  vy_max = (float)pos->r.ymax / dst_tex->height * 2 - 1;
	float vb[16];
	vb[0] = vx_min; vb[1] = vy_min;
	vb[4] = vx_min; vb[5] = vy_max;
	vb[8] = vx_max; vb[9] = vy_max;
	vb[12] = vx_max; vb[13] = vy_min;
	if (pos->is_rotated) {
		vb[2] = tx_max; vb[3] = ty_min;
		vb[6] = tx_min; vb[7] = ty_min;
		vb[10] = tx_min; vb[11] = ty_max;
		vb[14] = tx_max; vb[15] = ty_max;
	} else {
		vb[2] = tx_min; vb[3] = ty_min;
		vb[6] = tx_min; vb[7] = ty_max;
		vb[10] = tx_max; vb[11] = ty_max;
		vb[14] = tx_max; vb[15] = ty_min;
	}

	dtex_draw_to_texture(buf, tex, dst_tex, vb);

	return pos;
}

void 
dtexc3_preload_tex(struct dtex_c3* c3, struct dtex_raw_tex* tex, struct dtex_buffer* buf) {
	// todo sort

	// todo select dst texture
	struct dtex_texture* dst_tex = c3->textures[0];

	// insert
	struct dp_pos* pos = dtexpacker_add(dst_tex->packer, tex->width, tex->height, true);
	if (!pos) {
		return;
	}

	// draw
	// draw old tex to new 
	float tx_min = 0, tx_max = 1,
		  ty_min = 0, ty_max = 1;
	float vx_min = (float)pos->r.xmin / dst_tex->width * 2 - 1,
		  vx_max = (float)pos->r.xmax / dst_tex->width * 2 - 1,
		  vy_min = (float)pos->r.ymin / dst_tex->height * 2 - 1,
		  vy_max = (float)pos->r.ymax / dst_tex->height * 2 - 1;
	float vb[16];
	vb[0] = vx_min; vb[1] = vy_min; vb[2] = tx_min; vb[3] = ty_min;
	vb[4] = vx_min; vb[5] = vy_max; vb[6] = tx_min; vb[7] = ty_max;
	vb[8] = vx_max; vb[9] = vy_max; vb[10] = tx_max; vb[11] = ty_max;
	vb[12] = vx_max; vb[13] = vy_min; vb[14] = tx_max; vb[15] = ty_min;
	dtex_draw_to_texture(buf, tex, dst_tex, vb);

	// todo new_tex

	dtex_pool_remove(tex);
}

// static inline struct dtex_texture* 
// _query_tex_position(struct dtex_c3* c3, const char* name, int idx, struct dtex_rect** pos) {
// 	unsigned int hash = _hash_origin_pack(name);
// 	struct hash_node* hn = c3->hash[hash];
// 	while (hn) {
// 		struct dtex_node* dr = &hn->n;
// 		if (strcmp(name, dr->pkg->name) == 0 && idx == dr->raw_tex_idx) {
// 			*pos = &dr->dst_rect;
// 			return dr->dst_tex;
// 		}
// 		hn = hn->next_hash;
// 	}
// 
// 	pos = NULL;
// 	return NULL;
// }

//static inline void
//_relocate_rrp(struct dtex_c3* c3, struct dtex_package* pkg) {
//	assert(pkg->rrp_pkg);
//	for (int i = 0; i < pkg->tex_size; ++i) {
//		struct dtex_rect* pos;
//		struct dtex_texture* tex = _query_tex_position(c3, pkg->name, i, &pos);
//		assert(tex && pos);
//		dtex_rrp_relocate(pkg->rrp_pkg, i, tex, pos);
//	}
//}

struct dtex_package* 
dtex_c3_query_pkg(struct dtex_c3* c3, const char* name) {
	unsigned int idx = _hash_origin_pack(name);
	struct hash_node* hn = c3->hash[idx];
	while (hn) {
		struct dtex_node* dr = &hn->n;
		if (strcmp(name, dr->pkg->name) == 0) {
			return dr->pkg;
		}
		hn = hn->next_hash;
	}
	return NULL;
}

void
dtex_c3_query_map_info(struct dtex_c3* c3, const char* name, struct dtex_texture** textures, struct dtex_rect** regions) {
	unsigned int idx = _hash_origin_pack(name);
	struct hash_node* hn = c3->hash[idx];
	while (hn) {
		struct dtex_node* dr = &hn->n;
		if (strcmp(name, dr->pkg->name) == 0) {
			textures[dr->raw_tex_idx] = dr->dst_tex;
			regions[dr->raw_tex_idx] = &dr->dst_rect;
		}
		hn = hn->next_hash;
	}
}

void 
dtex_c3_debug_draw(struct dtex_c3* c3) {
// 	const float edge = 0.5f;
// 	for (int i = 0; i < c3->tex_size; ++i) {
// 		dtex_debug_draw_with_pos(c3->textures[i]->tex, 
// 			-1 + i * edge, 1 - edge, -1 + i * edge + edge, 1);
// 	}

	dtex_debug_draw(c3->textures[0]->tex);

    //dtex_debug_draw(5);
}