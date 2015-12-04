#include "dtex_c3.h"
#include "dtex_tp.h"
#include "dtex_shader.h"
#include "dtex_rrp.h"
#include "dtex_rrr.h"
#include "dtex_b4r.h"
#include "dtex_file.h"
#include "dtex_math.h"
#include "dtex_loader.h"
#include "dtex_package.h"
#include "dtex_ej_utility.h"
#include "dtex_texture_loader.h"
#include "dtex_async_loader.h"
#include "dtex_texture.h"
#include "dtex_res_cache.h"
#include "dtex_res_path.h"
#include "dtex_hash.h"
#include "dtex_log.h"
#include "dtex_array.h"
#include "dtex_debug.h"
#include "dtex_render.h"
#include "dtex_c3_strategy.h"

#include "ejoy2d.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MAX_TEX_SIZE 128

#define NODE_SIZE 512
#define PRELOAD_SIZE 512

#define SCALE_EVERTIME 0.9f
#define MIN_SCALE 0.1f

#define TOT_AREA_SCALE 1.0f

struct c3_node {
	struct dtex_package* pkg;
	// src
	int src_tex_idx;
	// dst
	struct dtex_texture* dst_tex;
	struct dtex_rect dst_rect;
	bool dst_rotated;

	bool finish;	// relocated
};

struct c3_prenode {
	struct dtex_package* pkg;
	int tex_idx;
	float scale;
};

struct tp_index {
	struct c3_node nodes[NODE_SIZE];
	int node_size;

	struct dtex_hash* hash;

	struct dtex_tp* tp;
};

struct dtex_c3 {
	int tex_edge;

	bool one_tex_mode;
	union {
		struct {
			struct dtex_texture* texture;
			struct tp_index s_index;
			struct tp_index d_index;
		} ONE;

		struct {
			struct dtex_texture* textures[MAX_TEX_SIZE];
			int tex_size;
			struct tp_index index;
		} MULTI;
	} t;

	// cache for dtex_c3_query_map_info
	struct dtex_array* tmp_array;

	int prenode_size;	
	struct c3_prenode prenodes[1];
};

struct dtex_c3* 
dtex_c3_create(int texture_size, bool one_tex_mode) {
	size_t sz = sizeof(struct dtex_c3) + sizeof(struct c3_prenode) * PRELOAD_SIZE;
	struct dtex_c3* c3 = (struct dtex_c3*)malloc(sz);
	if (!c3) {
		return NULL;
	}
	memset(c3, 0, sz);

	c3->tex_edge = texture_size;

	c3->one_tex_mode = one_tex_mode;
	if (one_tex_mode) {
		c3->t.ONE.texture = dtex_res_cache_fetch_mid_texture(texture_size);
		int half_sz = texture_size >> 1;

		c3->t.ONE.s_index.hash = dtex_hash_create(50, 50, 5, dtex_string_hash_func, dtex_string_equal_func);
		c3->t.ONE.s_index.tp = dtex_tp_create(texture_size, half_sz, PRELOAD_SIZE);

		c3->t.ONE.d_index.hash = dtex_hash_create(50, 50, 5, dtex_string_hash_func, dtex_string_equal_func);
		c3->t.ONE.d_index.tp = dtex_tp_create(texture_size, half_sz, PRELOAD_SIZE);
	} else {
		struct dtex_texture* tex = dtex_res_cache_fetch_mid_texture(texture_size);
		tex->t.MID.tp = dtex_tp_create(texture_size, texture_size, PRELOAD_SIZE);
		c3->t.MULTI.textures[c3->t.MULTI.tex_size++] = tex;

		c3->t.MULTI.index.hash = dtex_hash_create(50, 50, 5, dtex_string_hash_func, dtex_string_equal_func);
		c3->t.MULTI.index.tp = NULL;
	}

	c3->tmp_array = dtex_array_create(128, sizeof(struct c3_node));

	c3->prenode_size = 0;

	return c3;
}

void dtex_c3_release(struct dtex_c3* c3) {
	if (c3->one_tex_mode) {
		dtex_res_cache_return_mid_texture(c3->t.ONE.texture);

		dtex_hash_release(c3->t.ONE.s_index.hash);
		dtex_tp_release(c3->t.ONE.s_index.tp);

		dtex_hash_release(c3->t.ONE.d_index.hash);
		dtex_tp_release(c3->t.ONE.d_index.tp);
	} else {
		for (int i = 0; i < c3->t.MULTI.tex_size; ++i) {
			dtex_res_cache_return_mid_texture(c3->t.MULTI.textures[i]);
		}
		dtex_hash_release(c3->t.MULTI.index.hash);
	}

	dtex_array_release(c3->tmp_array);

	free(c3);
}

static inline void
_clear_tp_index(struct tp_index* index) {
	index->node_size = 0;

	dtex_hash_clear(index->hash);

	if (index->tp) {
		dtex_tp_clear(index->tp);
	}
}

void 
dtex_c3_clear(struct dtex_c3* c3) {
	if (c3->one_tex_mode && c3->t.ONE.d_index.node_size != 0) {
		dtex_texture_clear_part(c3->t.ONE.texture, 0, 0, 1, 0.5f);
		_clear_tp_index(&c3->t.ONE.d_index);
	}
}

static inline bool
_is_pkg_loaded(struct dtex_c3* c3, const char* pkg_name) {
	struct c3_node* node = NULL;
	if (c3->one_tex_mode) {
		node = (struct c3_node*)dtex_hash_query(c3->t.ONE.s_index.hash, (void*)pkg_name);
		if (!node) {
			node = (struct c3_node*)dtex_hash_query(c3->t.ONE.d_index.hash, (void*)pkg_name);
		}
	} else {
		node = (struct c3_node*)dtex_hash_query(c3->t.MULTI.index.hash, (void*)pkg_name);
	}
	return node != NULL;
}

void 
dtex_c3_load(struct dtex_c3* c3, struct dtex_package* pkg, float scale, bool force) {
	if (!force) {
		bool loaded = _is_pkg_loaded(c3, pkg->name);
		if (loaded) {
			return;
		}
	}

	for (int i = 0; i < pkg->texture_count; ++i) {
		if (c3->prenode_size == PRELOAD_SIZE) {
			dtex_warning("dtex_c3_load preload full");
			return;
		}
		struct c3_prenode* n = &c3->prenodes[c3->prenode_size++];
		n->pkg = pkg;
		n->tex_idx = i;
		n->scale = scale;
	}
}

static inline int
_compare_preload_name(const void *arg1, const void *arg2) {
	struct c3_prenode *node1, *node2;

	node1 = *((struct c3_prenode**)(arg1));
	node2 = *((struct c3_prenode**)(arg2));

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

static inline int
_compare_preload_length(const void *arg1, const void *arg2) {
	struct c3_prenode *node1, *node2;

	node1 = *((struct c3_prenode**)(arg1));
	node2 = *((struct c3_prenode**)(arg2));

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

static inline void
_get_unique_prenodes(struct dtex_c3* c3, struct c3_prenode** ret_set, int* ret_sz) {
	for (int i = 0; i < c3->prenode_size; ++i) {
		ret_set[i] = &c3->prenodes[i];
	}
	qsort((void*)ret_set, c3->prenode_size, sizeof(struct c3_prenode*), _compare_preload_name);

	struct c3_prenode* unique[PRELOAD_SIZE];
	unique[0] = ret_set[0];
	int unique_size = 1;
	for (int i = 1; i < c3->prenode_size; ++i) {
		struct c3_prenode* last = unique[unique_size-1];
		struct c3_prenode* curr = ret_set[i];
		if (_compare_preload_name(&curr, &last) == 0) {
			;
		} else {
			unique[unique_size++] = curr;
		}
	}
	memcpy(ret_set, unique, sizeof(struct c3_prenode*) * unique_size);
	*ret_sz = unique_size;	
}

static inline bool
_pack_preload_node(float scale, 
                   struct c3_prenode* pre_node, 
				   struct dtex_texture* texture,
				   struct tp_index* index,
				   float y_offset) {
	assert(texture->type == DTEX_TT_MID);

	struct dtex_texture* tex = pre_node->pkg->textures[pre_node->tex_idx];
	int w = tex->width * pre_node->scale * scale,
		h = tex->height * pre_node->scale * scale;
	struct dtex_tp_pos* pos = NULL;
	// todo padding
	if (w >= h) {
		pos = dtex_tp_add(index->tp, w, h, true);
	} else {
		pos = dtex_tp_add(index->tp, h, w, true);
	}
	if (!pos) {
		dtex_warning("+++++ c3 insert fail.");
		return false;
	}

	pos->r.ymin += y_offset;
	pos->r.ymax += y_offset;

	struct c3_node* node = NULL;
	if (index->node_size == NODE_SIZE) {
		dtex_warning("+++++ c3 nodes empty.");
		return false;
	}
	node = &index->nodes[index->node_size++];
	
	node->pkg = pre_node->pkg;
	node->src_tex_idx = pre_node->tex_idx;
	node->dst_tex = texture;
	node->dst_rect = pos->r;
	node->dst_rotated = (pos->is_rotated && w >= h) || (!pos->is_rotated && h >= w);
	node->finish = false;

	pos->ud = node;

	dtex_hash_insert(index->hash, pre_node->pkg->name, node, true);

	return true;
}

static inline bool
_pack_preload_list_with_scale(struct dtex_c3* c3, struct c3_prenode** pre_list, int pre_sz, float scale) {
	if (c3->one_tex_mode) {
		for (int i = 0; i < pre_sz; ++i) {
			struct c3_prenode* node = pre_list[i];
			bool succ = false;
			bool is_static = dtex_c3_is_static(node->pkg->c3_stg);
			if (is_static) {
				float y_offset = c3->t.ONE.texture->height * 0.5f;
				succ = _pack_preload_node(scale, node, c3->t.ONE.texture, &c3->t.ONE.s_index, y_offset);
			} else {
				succ = _pack_preload_node(scale, node, c3->t.ONE.texture, &c3->t.ONE.d_index, 0);
			}
			if (!succ) {
				return false;
			}
		}
		return true;
	} else {
		// init rect tp
		for (int i = 0; i < c3->t.MULTI.tex_size; ++i) {
			struct dtex_texture* tex = c3->t.MULTI.textures[i];
			assert(tex->type == DTEX_TT_MID);

			if (!tex->t.MID.tp) {
				tex->t.MID.tp = dtex_tp_create(tex->width, tex->height, c3->prenode_size + 100);
			}

	// 		if (tex->t.MID.tp) {
	// 			dtex_tp_release(tex->t.MID.tp);
	// 		}
	// 		// tp's capacity should larger for later inserting
	// 		tex->t.MID.tp = dtex_tp_create(tex->width, tex->height, c3->prenode_size + 100);
		}

		// insert
		int first_try_idx = 0;
		for (int i = 0; i < pre_sz; ++i) {
			struct c3_prenode* node = pre_list[i];
			bool success = false;
			for (int j = 0; j < c3->t.MULTI.tex_size; ++j) {
				struct dtex_texture* tex = c3->t.MULTI.textures[(first_try_idx+ j) % c3->t.MULTI.tex_size];
				success = _pack_preload_node(scale, node, tex, &c3->t.MULTI.index, 0);
				if (success) {
					first_try_idx = j;
					break;
				}
			}
			if (!success) {
				return false;
			}
		}
		return true;
	}
}

static inline float
_pack_nodes(struct dtex_c3* c3, struct c3_prenode** pre_list, int pre_sz, float alloc_scale) {
	qsort((void*)pre_list, pre_sz, sizeof(struct c3_prenode*), _compare_preload_length);

	float scale = alloc_scale;
	while (scale > MIN_SCALE) {
		bool success = _pack_preload_list_with_scale(c3, pre_list, pre_sz, scale);
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
	struct c3_node *node1, *node2;

	node1 = *((struct c3_node**)(arg1));
	node2 = *((struct c3_node**)(arg2));

	return node1->pkg < node2->pkg;
}

// static inline int
// _compare_dr_dst_tex(const void *arg1, const void *arg2) {
// 	struct c3_node *node1, *node2;
// 
// 	node1 = *((struct c3_node**)(arg1));
// 	node2 = *((struct c3_node**)(arg2));
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
_relocate_pic(int pic_id, struct ej_pack_picture* ej_pic, void* ud) {
	struct c3_node* dr = (struct c3_node*)ud;
	int tex_uid = dr->pkg->textures[dr->src_tex_idx]->uid;
	struct dtex_texture* src = dtex_texture_fetch(tex_uid);
	for (int i = 0; i < ej_pic->n; ++i) {
		struct pack_quad* ej_q = &ej_pic->rect[i];
		if (ej_q->texid != tex_uid) {
			continue;
		}

		ej_q->texid = dr->dst_tex->uid;
		for (int j = 0; j < 4; ++j) {
			float x = (float)ej_q->texture_coord[j*2]   * src->inv_width;
			float y = (float)ej_q->texture_coord[j*2+1] * src->inv_height;
			if (src->type == DTEX_TT_RAW && src->t.RAW.lod_scale != 1) {
				x *= src->t.RAW.lod_scale;
				y *= src->t.RAW.lod_scale;
			}
			ej_q->texture_coord[j*2]   = dr->dst_rect.xmin + (dr->dst_rect.xmax - dr->dst_rect.xmin) * x;
			ej_q->texture_coord[j*2+1] = dr->dst_rect.ymin + (dr->dst_rect.ymax - dr->dst_rect.ymin) * y;
		}
	}
}

// todo other format: rrr, b4r

static inline void
_relocate_node(struct dtex_texture* src, struct c3_node* dst) {
	// draw old tex to new 
	float tx_min = 0, tx_max = 1,
		ty_min = 0, ty_max = 1;
	float vx_min = (float)dst->dst_rect.xmin * dst->dst_tex->inv_width  * 2 - 1,
		  vx_max = (float)dst->dst_rect.xmax * dst->dst_tex->inv_width  * 2 - 1,
		  vy_min = (float)dst->dst_rect.ymin * dst->dst_tex->inv_height * 2 - 1,
		  vy_max = (float)dst->dst_rect.ymax * dst->dst_tex->inv_height * 2 - 1;
	float vb[16];
	vb[0] = vx_min; vb[1] = vy_min; vb[2] = tx_min; vb[3] = ty_min;
	vb[4] = vx_min; vb[5] = vy_max; vb[6] = tx_min; vb[7] = ty_max;
	vb[8] = vx_max; vb[9] = vy_max; vb[10] = tx_max; vb[11] = ty_max;
	vb[12] = vx_max; vb[13] = vy_min; vb[14] = tx_max; vb[15] = ty_min;
	dtex_draw_to_texture(src, dst->dst_tex, vb);

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
	struct c3_node* node = (struct c3_node*)ud;
	struct dtex_texture* tex = node->pkg->textures[node->src_tex_idx];

	bool tex_loaded = false;
	if (tex->id == 0) {
		tex_loaded = false;
		dtex_load_texture_all(is, tex);
	} else {
		tex_loaded = true;
	}

	dtex_draw_begin();
	_relocate_node(tex, node);
	dtex_draw_end();

	if (!tex_loaded) {
		dtex_package_remove_texture_ref(node->pkg, tex);
		dtex_texture_release(tex);
	}

	--node->pkg->c3_loading;
}

static inline void
_relocate_nodes(struct dtex_c3* c3, struct dtex_loader* loader, bool async) {
	// sort all node by its texture
	int node_sz = 0;
	struct c3_node* nodes[NODE_SIZE];
	if (c3->one_tex_mode) {
		for (int i = 0; i < c3->t.ONE.s_index.node_size; ++i) {
			struct c3_node* node = &c3->t.ONE.s_index.nodes[i];
			if (!node->finish) {
				nodes[node_sz++] = node;
			}
		}
		for (int i = 0; i < c3->t.ONE.d_index.node_size; ++i) {
			struct c3_node* node = &c3->t.ONE.d_index.nodes[i];
			if (!node->finish) {
				nodes[node_sz++] = node;
			}
		}
	} else {
		for (int i = 0; i < c3->t.MULTI.index.node_size; ++i) {
			struct c3_node* node = &c3->t.MULTI.index.nodes[i];
			if (!node->finish) {
				nodes[node_sz++] = node;
			}
		}
	}
	qsort((void*)nodes, node_sz, sizeof(struct c3_node*), _compare_dr_ori_pkg);

	// draw
	struct dtex_package* last_pkg = NULL;
	for (int i = 0; i < node_sz; ++i) {
		struct c3_node* node = nodes[i];
		node->finish = true;

		struct dtex_package* pkg = node->pkg;

		// change package should flush shader, as texture maybe removed
		if (last_pkg != NULL && pkg != last_pkg) {
//			dtex_shader_flush();
		}

		// load old tex

		bool tex_loaded = false;
		struct dtex_texture* ori_tex = NULL;
		if (pkg->rrr_pkg) {
//			ori_tex = dtex_rrr_load_tex(pkg->rrr_pkg, pkg, node->raw_tex_idx);
		} else if (pkg->b4r_pkg) {
//			ori_tex = dtex_b4r_load_tex(pkg->b4r_pkg, pkg, node->raw_tex_idx);
		} else {
			ori_tex = pkg->textures[node->src_tex_idx];
			assert(ori_tex && ori_tex->type == DTEX_TT_RAW);

			int pkg_idx = dtex_package_texture_idx(pkg, ori_tex);
			assert(pkg_idx != -1);
			if (!async) {
				if (ori_tex->id == 0) {
					tex_loaded = false;
					dtex_load_texture(loader, pkg, pkg_idx);
				} else {
					tex_loaded = true;
				}
			} else {
				++pkg->c3_loading;
				const char* path = dtex_get_img_filepath(pkg->rp, pkg_idx, pkg->LOD);
 				dtex_async_load_file(path, _relocate_nodes_cb, node, "c3");
 			}
		}

 		if (!async) {
 			_relocate_node(ori_tex, node);
 			if (!tex_loaded) {
				dtex_package_remove_texture_ref(pkg, ori_tex);
				dtex_texture_release(ori_tex);
 			}
 		}

		last_pkg = pkg;
	}
}

static inline float
_alloc_texture(struct dtex_c3* c3, struct c3_prenode** pre_list, int pre_sz) {
	if (c3->one_tex_mode) {
		return 1;
	}

	float area = 0;
	for (int i = 0; i < pre_sz; ++i) {
		struct c3_prenode* n = pre_list[i];
		struct dtex_texture* tex = n->pkg->textures[n->tex_idx];
		int w = tex->width * n->scale,
			h = tex->height * n->scale;
		area += w * h;
	}
	area *= TOT_AREA_SCALE;	

 	for (int i = 0; i < c3->t.MULTI.tex_size; ++i) {
 		area -= dtex_tp_get_free_space(c3->t.MULTI.textures[i]->t.MID.tp);
 	}
 	if (area <= 0) {
 		return 1.0f;
 	}

	while (area > 0) {
		if (c3->t.MULTI.tex_size == MAX_TEX_SIZE) {
			dtex_fault("c3 texture full.");
		}
		struct dtex_texture* tex = dtex_res_cache_fetch_mid_texture(c3->tex_edge);
		c3->t.MULTI.textures[c3->t.MULTI.tex_size++] = tex;
		area -= c3->tex_edge * c3->tex_edge;
	}

	// todo: 根据可用内存大小进行缩放
	return 1.0f;
}

void 
dtex_c3_load_end(struct dtex_c3* c3, struct dtex_loader* loader, bool async) {
	if (c3->prenode_size == 0) {
		return;
	}

	struct c3_prenode* unique_set[c3->prenode_size];
	int unique_sz = 0;
	_get_unique_prenodes(c3, unique_set, &unique_sz);

	float alloc_scale = _alloc_texture(c3, unique_set, unique_sz);

	/*float scale = */_pack_nodes(c3, unique_set, unique_sz, alloc_scale);

	dtex_draw_begin();
	_relocate_nodes(c3, loader, async);
	dtex_draw_end();

	c3->prenode_size = 0;
}

//struct dtex_tp_pos* 
//dtex_c3_load_tex(struct dtex_c3* c3, struct dtex_texture* tex, struct dtex_texture** dst) {
//	// todo sort
//
//	// todo select dst texture
//	struct dtex_texture* dst_tex = c3->textures[0];
//	assert(dst_tex->type == DTEX_TT_MID);
//	*dst = dst_tex;
//
//	// insert
//	struct dtex_tp_pos* pos = dtex_tp_add(dst_tex->t.MID.tp, tex->width, tex->height, true);
//	if (!pos) {
//		return NULL;
//	}
//
//	// draw
//	// draw old tex to new 
//	float tx_min = 0, tx_max = 1,
//		  ty_min = 0, ty_max = 1;
//	float vx_min = (float)pos->r.xmin * dst_tex->inv_width  * 2 - 1,
//		  vx_max = (float)pos->r.xmax * dst_tex->inv_width  * 2 - 1,
//		  vy_min = (float)pos->r.ymin * dst_tex->inv_height * 2 - 1,
//		  vy_max = (float)pos->r.ymax * dst_tex->inv_height * 2 - 1;
//	float vb[16];
//	vb[0]  = vx_min; vb[1]  = vy_min;
//	vb[4]  = vx_min; vb[5]  = vy_max;
//	vb[8]  = vx_max; vb[9]  = vy_max;
//	vb[12] = vx_max; vb[13] = vy_min;
//	if (pos->is_rotated) {
//		vb[2]  = tx_max; vb[3]  = ty_min;
//		vb[6]  = tx_min; vb[7]  = ty_min;
//		vb[10] = tx_min; vb[11] = ty_max;
//		vb[14] = tx_max; vb[15] = ty_max;
//	} else {
//		vb[2]  = tx_min; vb[3]  = ty_min;
//		vb[6]  = tx_min; vb[7]  = ty_max;
//		vb[10] = tx_max; vb[11] = ty_max;
//		vb[14] = tx_max; vb[15] = ty_min;
//	}
//
//	dtex_draw_before();
//	dtex_draw_to_texture(tex, dst_tex, vb);
//	dtex_draw_after();
//
//	return pos;
//}

// void 
// dtexc3_preload_tex(struct dtex_c3* c3, struct dtex_texture* tex) {
// 	// todo sort
// 
// 	// todo select dst texture
// 	struct dtex_texture* dst_tex = c3->textures[0];
// 	assert(dst_tex->type == DTEX_TT_MID);
// 
// 	// insert
// 	struct dtex_tp_pos* pos = dtex_tp_add(dst_tex->t.MID.tp, tex->width, tex->height, true);
// 	if (!pos) {
// 		return;
// 	}
// 
// 	// draw
// 	// draw old tex to new 
// 	float tx_min = 0, tx_max = 1,
// 		  ty_min = 0, ty_max = 1;
// 	float vx_min = (float)pos->r.xmin * dst_tex->inv_width  * 2 - 1,
// 		  vx_max = (float)pos->r.xmax * dst_tex->inv_width  * 2 - 1,
// 		  vy_min = (float)pos->r.ymin * dst_tex->inv_height * 2 - 1,
// 		  vy_max = (float)pos->r.ymax * dst_tex->inv_height * 2 - 1;
// 	float vb[16];
// 	vb[0] = vx_min; vb[1] = vy_min; vb[2] = tx_min; vb[3] = ty_min;
// 	vb[4] = vx_min; vb[5] = vy_max; vb[6] = tx_min; vb[7] = ty_max;
// 	vb[8] = vx_max; vb[9] = vy_max; vb[10] = tx_max; vb[11] = ty_max;
// 	vb[12] = vx_max; vb[13] = vy_min; vb[14] = tx_max; vb[15] = ty_min;
// 	dtex_draw_to_texture(tex, dst_tex, vb);
// 
// 	// todo new_tex
// 
// 	dtex_raw_tex_release(tex);
// }

// static inline struct dtex_texture* 
// _query_tex_position(struct dtex_c3* c3, const char* name, int idx, struct dtex_rect** pos) {
// 	unsigned int hash = _hash_origin_pack(name);
// 	struct hash_node* hn = c3->hash[hash];
// 	while (hn) {
// 		struct c3_node* dr = &hn->n;
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

void
dtex_c3_query_map_info(struct dtex_c3* c3, struct dtex_package* pkg, struct dtex_texture** textures, struct dtex_rect** regions) {
	if (c3->one_tex_mode) {
		dtex_hash_query_all(c3->t.ONE.s_index.hash, pkg->name, c3->tmp_array);
		dtex_hash_query_all(c3->t.ONE.d_index.hash, pkg->name, c3->tmp_array);
	} else {
		dtex_hash_query_all(c3->t.MULTI.index.hash, pkg->name, c3->tmp_array);
	}

	int sz = dtex_array_size(c3->tmp_array);
	for (int i = 0; i < sz; ++i) {
		struct c3_node* node = *(struct c3_node**)dtex_array_fetch(c3->tmp_array, i);
		textures[node->src_tex_idx] = node->dst_tex;
		regions[node->src_tex_idx]  = &node->dst_rect;		
	}
	dtex_array_clear(c3->tmp_array);
}

void 
dtex_c3_debug_draw(struct dtex_c3* c3) {
	if (c3->one_tex_mode) {
		dtex_debug_draw(c3->t.ONE.texture->id, 1);
	} else {
		dtex_debug_draw(c3->t.MULTI.textures[0]->id, 1);
		if (c3->t.MULTI.tex_size > 1) {
			dtex_debug_draw(c3->t.MULTI.textures[1]->id, 2);
		}
	}
}