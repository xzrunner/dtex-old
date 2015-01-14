#include "dtex_c3.h"
#include "dtex_packer.h"
#include "dtex_texture.h"
#include "dtex_loader.h"
#include "dtex_draw.h"
#include "dtex_buffer.h"

#include "package.h"
#include "platform.h"

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
	struct dtex_package* package;
	int raw_tex_idx;
	// dest info
	struct dtex_texture* dst_tex;
	struct dtex_rect dst_rect;
	int16_t is_rotated;
};

struct hash_node {
	struct hash_node* next_hash;
	struct dtex_node n;
};

struct preload_node {
	struct dtex_package* package;
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
dtexc3_create() {
	size_t nsize = NODE_SIZE * sizeof(struct hash_node);
	size_t psize = PRELOAD_SIZE * sizeof(struct preload_node);
	size_t sz = sizeof(struct dtex_c3) + nsize + psize;
	struct dtex_c3* dtex = (struct dtex_c3*)malloc(sz);
	memset(dtex, 0, sz);

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

void dtexc3_release(struct dtex_c3* dtex, struct dtex_buffer* buf) {
	for (int i = 0; i < dtex->tex_size; ++i) {
		dtex_del_tex(buf, dtex->textures[i]);
	}

	free(dtex);
}

void 
dtexc3_preload(struct dtex_c3* dtex, struct dtex_package* pkg, float scale) {
	for (int i = 0; i < dtex->preload_size; ++i) {
		if (pkg == dtex->preload_list[i]->package) {
			return;
		}
	}

	for (int i = 0; i < pkg->tex_size; ++i) {
		assert(dtex->preload_size <= MAX_TEX_SIZE);
		struct preload_node* n = dtex->preload_list[dtex->preload_size++];
		n->package = pkg;
		n->raw_tex_idx = i;
		n->scale = scale;
	}
}

static inline int
_compare_preload_name(const void *arg1, const void *arg2) {
	struct preload_node *node1, *node2;

	node1 = *((struct preload_node**)(arg1));
	node2 = *((struct preload_node**)(arg2));

	int cmp = strcmp(node1->package->name, node2->package->name);
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

	int w1 = node1->package->textures[node1->raw_tex_idx].width,
		h1 = node1->package->textures[node1->raw_tex_idx].height;
	int w2 = node2->package->textures[node2->raw_tex_idx].width,
		h2 = node2->package->textures[node2->raw_tex_idx].height;	

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
_unique_preload_list(struct dtex_c3* dtex) {
	qsort((void*)dtex->preload_list, dtex->preload_size, sizeof(struct preload_node*), _compare_preload_name);
	struct preload_node* unique[PRELOAD_SIZE];
	unique[0] = dtex->preload_list[0];
	int unique_size = 1;
	for (int i = 1; i < dtex->preload_size; ++i) {
		struct preload_node* last = unique[unique_size-1];
		struct preload_node* curr = dtex->preload_list[i];
		if (strcmp(curr->package->name, last->package->name) == 0 && curr->raw_tex_idx == last->raw_tex_idx) {
			;
		} else {
			unique[unique_size] = curr;
			++unique_size;
		}
	}
	memcpy(dtex->preload_list, unique, unique_size*sizeof(struct preload_node*));
	dtex->preload_size = unique_size;	
}

static inline struct hash_node* 
_new_hash_rect(struct dtex_c3* dtex) {
	if (dtex->freelist == NULL) {
		return NULL;  	
	}
	struct hash_node* ret = dtex->freelist;
	dtex->freelist = ret->next_hash;
	assert(ret != NULL);
	return ret;
}

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
_pack_preload_node(struct dtex_c3* dtex, float scale, struct preload_node* node, struct dtex_texture* texture) {
	int w = node->package->textures[node->raw_tex_idx].width * node->scale * scale,
		h = node->package->textures[node->raw_tex_idx].height * node->scale * scale;
	struct dp_position* pos = NULL;
	// todo padding
	if (w >= h) {
		pos = dtexpacker_add(texture->packer, w, h);
	} else {
		pos = dtexpacker_add(texture->packer, h, w);
	}
	if (!pos) {
		return false;
	}

	struct hash_node* hn = _new_hash_rect(dtex);
	hn->n.package = node->package;
	hn->n.raw_tex_idx = node->raw_tex_idx;
	hn->n.dst_tex = texture;
	hn->n.dst_rect = pos->r;
	if ((pos->is_rotated && w >= h) ||
		(!pos->is_rotated && h >= w)) {
		hn->n.is_rotated = 1;
	}
	pos->ud = &hn->n;

	unsigned int idx = _hash_origin_pack(node->package->name);
	hn->next_hash = dtex->hash[idx];
	dtex->hash[idx] = hn;	

	return true;
}

static inline bool
_pack_preload_list_with_scale(struct dtex_c3* dtex, float scale) {
	// init rect packer
	for (int i = 0; i < dtex->tex_size; ++i) {
		struct dtex_texture* tex = dtex->textures[i];
		if (tex) {
			dtexpacker_release(tex->packer);
		}
		tex->packer = dtexpacker_create(tex->width, tex->height, dtex->preload_size);
	}
	// insert
	int tex_idx = 0;
	for (int i = 0; i < dtex->preload_size; ++i) {
		struct preload_node* node = dtex->preload_list[i];
		bool success = false;
		for (int j = tex_idx; j < tex_idx + dtex->tex_size && !success; ++j) {
			struct dtex_texture* tex = dtex->textures[j % dtex->tex_size];
			success = _pack_preload_node(dtex, scale, node, tex);
			tex_idx = j;
		}
		if (!success) {
			return false;
		}
	}	
	return true;
}

static inline float
_pack_preload_list(struct dtex_c3* dtex, float alloc_scale) {
	qsort((void*)dtex->preload_list, dtex->preload_size, 
		sizeof(struct _pack_preload_list*), _compare_preload_length);

	float scale = alloc_scale;
	while (scale > MIN_SCALE) {
		bool success = _pack_preload_list_with_scale(dtex, scale);
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

	return node1->package < node2->package;
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

static inline void
_draw_preload_list(struct dtex_c3* dtex, float scale, struct dtex_loader* loader, struct dtex_buffer* buf) {
	// sort all node by its texture
	int count = 0;
	struct dtex_node* nodes[NODE_SIZE];
	for (int i = 0; i < HASH_SIZE; ++i) {
		struct hash_node* hn = dtex->hash[i];
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
		if (last_pkg != NULL && dr->package != last_pkg) {
			dtex_flush_shader();
		}

		// load old tex
		struct dtex_raw_tex* ori_tex = dtexloader_load_texture(loader, dr->package, dr->raw_tex_idx);

		// draw old tex to new 
		float tx_min = 0, tx_max = 1,
		      ty_min = 0, ty_max = 1;
		float vx_min = (float)dr->dst_rect.xmin / dr->dst_tex->width * 2 - 1,
		      vx_max = (float)dr->dst_rect.xmax / dr->dst_tex->width * 2 - 1,
		      vy_min = (float)dr->dst_rect.ymin / dr->dst_tex->height * 2 - 1,
		      vy_max = (float)dr->dst_rect.ymax / dr->dst_tex->height * 2 - 1;
		float vb[16];
		vb[0] = vx_min; vb[1] = vy_min; vb[2] = tx_min; vb[3] = ty_min;
		vb[4] = vx_min; vb[5] = vy_max; vb[6] = tx_min; vb[7] = ty_max;
		vb[8] = vx_max; vb[9] = vy_max; vb[10] = tx_max; vb[11] = ty_max;
		vb[12] = vx_max; vb[13] = vy_min; vb[14] = tx_max; vb[15] = ty_min;
		dtex_draw_to_texture(buf, ori_tex, vb, dr->dst_tex);
        
        dtexloader_unload_texture(ori_tex);

		last_pkg = dr->package;
	}
}

static inline float
_alloc_texture(struct dtex_c3* dtex, struct dtex_buffer* buf) {
	float area = 0;
	for (int i = 0; i < dtex->preload_size; ++i) {
		struct preload_node* n = dtex->preload_list[i];
		int w = n->package->textures[n->raw_tex_idx].width * n->scale,
			h = n->package->textures[n->raw_tex_idx].height * n->scale;
		area += w * h;
	}
	area *= TOT_AREA_SCALE;	

	int gain = dtexbuf_reserve(buf, area);
	int real = 0;
	do {
		struct dtex_texture* tex = dtex_new_tex(buf);
		real += tex->width * tex->height;
		dtex->textures[dtex->tex_size++] = tex;
	} while (real < gain);

	return MIN(1.0f, (float)gain / area);
}

void 
dtexc3_preload_end(struct dtex_c3* dtex, struct dtex_loader* loader, struct dtex_buffer* buf) {
	if (dtex->preload_size == 0) {
		return;
	}

	_unique_preload_list(dtex);	

	float alloc_scale = _alloc_texture(dtex, buf);

	float scale = _pack_preload_list(dtex, alloc_scale);

	_draw_preload_list(dtex, scale, loader, buf);
    
    dtex->preload_size = 0;
}

void 
dtexc3_relocate(struct dtex_c3* dtex, struct dtex_package* pkg) {
	pkg->ej_pkg->texture_n = pkg->tex_size;
	for (int i = 0; i < pkg->tex_size; ++i) {
		pkg->ej_pkg->tex[i].id = 0;
	}

	unsigned int idx = _hash_origin_pack(pkg->name);
	struct hash_node* pkg_hn = dtex->hash[idx];

	struct ejoypic* ep = pkg->ej_pkg->ep;
	for (int id = 0; id < ep->max_id; ++id) {
		struct animation * ani = ep->spr[id];
		if (ani == NULL || ani->part_n > 0) {
			continue;
		}

		struct picture* pic = (struct picture*)ani;
		for (int j = 0; j < -pic->n; ++j) {
			struct picture_part* part = &pic->part[j];
			struct dtex_raw_tex* t = &pkg->textures[part->texid];
			// find dtex_node
			struct dtex_node* to_node = NULL;
			struct hash_node* hn = pkg_hn;
			while (hn) {
				struct dtex_node* n = &hn->n;
				if (strcmp(pkg->name, n->package->name) == 0 && part->texid == n->raw_tex_idx) {
					to_node = n;
					break;
				}
				hn = hn->next_hash;
			}
			assert(to_node);
			// map part to dest texture
			assert(part->texid < pkg->ej_pkg->texture_n);
			struct texture* ori_tex = &pkg->ej_pkg->tex[part->texid];
			if (ori_tex->id == 0) {
				ori_tex->id = to_node->dst_tex->tex;
				ori_tex->id_alpha = 0;
				ori_tex->width = 1.0f /  to_node->dst_tex->width;
				ori_tex->height = 1.0f /  to_node->dst_tex->height;	
				ori_tex->format = TEXTURE8;			
			}
			struct dtex_rect* to_rect = &to_node->dst_rect;
			for (int i = 0; i < 4; ++i) {
				float x = (float)part->src[i*2] / t->width;
				float y = (float)part->src[i*2+1] / t->height;
				part->src[i*2] = to_rect->xmin + (to_rect->xmax - to_rect->xmin) * x;
				part->src[i*2+1] = to_rect->ymin + (to_rect->ymax - to_rect->ymin) * y;
			}
		}
	}
}

struct dtex_package* 
dtexc3_query_package(struct dtex_c3* dtex, const char* name) {
	unsigned int idx = _hash_origin_pack(name);
	struct hash_node* hn = dtex->hash[idx];
	while (hn) {
		struct dtex_node* dr = &hn->n;
		if (strcmp(name, dr->package->name) == 0) {
			return dr->package;
		}
		hn = hn->next_hash;
	}
	return NULL;
}

struct dtex_rect*
dtexc3_query_rect(struct dtex_c3* dtex, const char* name) {
	unsigned int idx = _hash_origin_pack(name);
	struct hash_node* hn = dtex->hash[idx];
	while (hn) {
		struct dtex_node* dr = &hn->n;
		if (strcmp(name, dr->package->name) == 0) {
			return &dr->dst_rect;
		}
		hn = hn->next_hash;
	}
	return NULL;
}

void 
dtexc3_debug_draw(struct dtex_c3* dtex) {
    dtex_debug_draw(dtex->textures[0]->tex);
    //dtex_debug_draw(5);
}