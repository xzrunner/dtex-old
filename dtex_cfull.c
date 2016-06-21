#include "dtex_cfull.h"
#include "dtex_package.h"
#include "dtex_texture.h"
#include "dtex_tp.h"
#include "dtex_render.h"
#include "dtex_ej_utility.h"

#include <ds_hash.h>
#include <logger.h>

#include <string.h>
#include <stdlib.h>
#include <assert.h>

inline int
dtex_cf_prenode_size_cmp(const void* arg1, const void* arg2) {
	struct dtex_cf_prenode *node1, *node2;

	node1 = *((struct dtex_cf_prenode**)(arg1));
	node2 = *((struct dtex_cf_prenode**)(arg2));

	int w1 = node1->pkg->textures[node1->tex_idx]->width,
		h1 = node1->pkg->textures[node1->tex_idx]->height;
	int w2 = node2->pkg->textures[node2->tex_idx]->width,
		h2 = node2->pkg->textures[node2->tex_idx]->height;

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

inline int
dtex_cf_node_pkg_cmp(const void* arg1, const void* arg2) {
	struct dtex_cf_node *node1, *node2;

	node1 = *((struct dtex_cf_node**)(arg1));
	node2 = *((struct dtex_cf_node**)(arg2));

	return node1->pkg < node2->pkg;
}

static inline int
_preload_name_cmp(const void* arg1, const void* arg2) {
	struct dtex_cf_prenode *node1, *node2;

	node1 = *((struct dtex_cf_prenode**)(arg1));
	node2 = *((struct dtex_cf_prenode**)(arg2));

	int cmp = strcmp(node1->pkg->name, node2->pkg->name);
	if (cmp != 0) {
		return cmp;
	}

	if (node1->pkg->textures[node1->tex_idx]->uid < node2->pkg->textures[node2->tex_idx]->uid) {
		return -1;
	} else if (node1->pkg->textures[node1->tex_idx]->uid > node2->pkg->textures[node2->tex_idx]->uid) {
		return 1;
	} else {
		return 0;
	}
}

void 
dtex_cf_unique_prenodes(struct dtex_cf_prenode* src_list, int src_sz,
                        struct dtex_cf_prenode** dst_list, int* dst_sz) {
	for (int i = 0; i < src_sz; ++i) {
		dst_list[i] = &src_list[i];
	}
	qsort((void*)dst_list, src_sz, sizeof(struct dtex_cf_prenode*), _preload_name_cmp);

	struct dtex_cf_prenode* unique[src_sz];
	unique[0] = dst_list[0];
	int unique_size = 1;
	for (int i = 1; i < src_sz; ++i) {
		struct dtex_cf_prenode* last = unique[unique_size-1];
		struct dtex_cf_prenode* curr = dst_list[i];
		if (_preload_name_cmp(&curr, &last) == 0) {
			;
		} else {
			unique[unique_size++] = curr;
		}
	}
	memcpy(dst_list, unique, sizeof(struct dtex_cf_prenode*) * unique_size);
	*dst_sz = unique_size;		
}

bool 
dtex_cf_pack_prenodes(struct dtex_cf_prenode* prenode, struct dtex_cf_texture* cf_tex, float scale) {
	assert(cf_tex->tex->type == DTEX_TT_MID);

	struct dtex_texture* tex = prenode->pkg->textures[prenode->tex_idx];
	int w = tex->width * prenode->scale * scale,
		h = tex->height * prenode->scale * scale;
	struct dtex_tp_pos* pos = NULL;
	// todo padding
	if (w >= h) {
		pos = dtex_tp_add(cf_tex->tp, w, h, true);
	} else {
		pos = dtex_tp_add(cf_tex->tp, h, w, false);
	}
	if (!pos) {
		LOGW("%s", "cf insert fail.");
		return false;
	}

	pos->r.xmin += cf_tex->region.xmin;
	pos->r.xmax += cf_tex->region.xmin;
	pos->r.ymin += cf_tex->region.ymin;
	pos->r.ymax += cf_tex->region.ymin;

	struct dtex_cf_node* node = NULL;
	if (cf_tex->node_count == DTEX_CF_MAX_NODE_COUNT) {
		LOGW("%s", "cf nodes full.");
		return false;
	}
	node = &cf_tex->nodes[cf_tex->node_count++];

	node->pkg = prenode->pkg;
	node->src_tex_idx = prenode->tex_idx;
	node->dst_tex = cf_tex;
	node->dst_rect = pos->r;
	node->dst_rotated = (pos->is_rotated && w >= h) || (!pos->is_rotated && h >= w);
	node->finish = false;

	pos->ud = node;

	ds_hash_insert(cf_tex->hash, prenode->pkg->name, node, true);

	return true;	
}

inline void
dtex_cf_clear_tex_info(struct dtex_cf_texture* tex) {
	tex->node_count = 0;
	assert(tex->hash);
	ds_hash_clear(tex->hash);
	assert(tex->tp);
	dtex_tp_clear(tex->tp);
}

inline void
dtex_cf_relocate_pic(int pic_id, struct ej_pack_picture* ej_pic, void* ud) {
	struct dtex_cf_node* node = (struct dtex_cf_node*)ud;
	int tex_uid = node->pkg->textures[node->src_tex_idx]->uid;
	struct dtex_texture* src = dtex_texture_fetch(tex_uid);
	for (int i = 0; i < ej_pic->n; ++i) {
		struct pack_quad* ej_q = &ej_pic->rect[i];
		if (ej_q->texid != tex_uid) {
			continue;
		}

		ej_q->texid = node->dst_tex->tex->uid;
		for (int j = 0; j < 4; ++j) {
			float x = (float)ej_q->texture_coord[j*2]   * src->inv_width;
			float y = (float)ej_q->texture_coord[j*2+1] * src->inv_height;
			if (src->type == DTEX_TT_RAW && src->t.RAW.lod_scale != 1) {
				x *= src->t.RAW.lod_scale;
				y *= src->t.RAW.lod_scale;
			}
			ej_q->texture_coord[j*2]   = node->dst_rect.xmin + (node->dst_rect.xmax - node->dst_rect.xmin) * x;
			ej_q->texture_coord[j*2+1] = node->dst_rect.ymin + (node->dst_rect.ymax - node->dst_rect.ymin) * y;
		}
	}
}

inline void
dtex_cf_relocate_node(struct dtex_texture* src, struct dtex_cf_node* dst) {
	// draw old tex to new 
	float tx_min = 0, tx_max = 1,
		  ty_min = 0, ty_max = 1;
	float vx_min = (float)dst->dst_rect.xmin * dst->dst_tex->tex->inv_width  * 2 - 1,
		  vx_max = (float)dst->dst_rect.xmax * dst->dst_tex->tex->inv_width  * 2 - 1,
		  vy_min = (float)dst->dst_rect.ymin * dst->dst_tex->tex->inv_height * 2 - 1,
		  vy_max = (float)dst->dst_rect.ymax * dst->dst_tex->tex->inv_height * 2 - 1;
	float vb[16];
	vb[0] = vx_min; vb[1] = vy_min; vb[2] = tx_min; vb[3] = ty_min;
	vb[4] = vx_min; vb[5] = vy_max; vb[6] = tx_min; vb[7] = ty_max;
	vb[8] = vx_max; vb[9] = vy_max; vb[10] = tx_max; vb[11] = ty_max;
	vb[12] = vx_max; vb[13] = vy_min; vb[14] = tx_max; vb[15] = ty_min;
	// todo for c4 ...
	dtex_draw_to_texture(src, dst->dst_tex->tex, vb);

	dtex_ej_pkg_traverse(dst->pkg->ej_pkg, dtex_cf_relocate_pic, dst);

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