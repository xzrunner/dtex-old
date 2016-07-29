#include "dtex_c3.h"
#include "dtex_cfull.h"
#include "dtex_tp.h"
#include "dtex_loader.h"
#include "dtex_package.h"
#include "dtex_ej_utility.h"
#include "dtex_texture_loader.h"
#include "dtex_async_loader.h"
#include "dtex_texture.h"
#include "dtex_res_cache.h"
#include "dtex_res_path.h"
#include "dtex_render.h"
#include "dtex_c3_strategy.h"
#include "dtex_debug.h"
#include "dtex_shader.h"

#include "dtex_facade.h"

#include <logger.h>
#include <fault.h>
#include <ds_array.h>
#include <ds_hash.h>

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MULTI_TEX_COUNT			1
#define MULTI_TEX_STATIC_COUNT	0

#define MAX_NODE_COUNT			512
#define MAX_PRELOAD_COUNT		512

#define SCALE_EVERTIME			0.9f
#define MIN_SCALE				0.1f

#define TOT_AREA_SCALE			1.0f

struct dtex_c3 {
	int tex_edge;

	bool one_tex_mode;
	union {
		struct {
			struct dtex_cf_texture s_up_tex;
			struct dtex_cf_texture d_down_tex;
		} ONE;
		struct {
			struct dtex_cf_texture textures[MULTI_TEX_COUNT];
		} MULTI;
	} t;

	// cache for dtex_c3_query_map_info
	struct ds_array* tmp_array;

	int prenode_size;	
	struct dtex_cf_prenode prenodes[1];
};

struct dtex_c3* 
dtex_c3_create(int texture_size, bool one_tex_mode) {
	size_t sz = sizeof(struct dtex_c3) + sizeof(struct dtex_cf_prenode) * MAX_PRELOAD_COUNT;
	struct dtex_c3* c3 = (struct dtex_c3*)malloc(sz);
	if (!c3) {
		return NULL;
	}
	memset(c3, 0, sz);

	c3->tex_edge = texture_size;

	c3->one_tex_mode = one_tex_mode;
	if (one_tex_mode) {
		struct dtex_texture* tex = dtex_res_cache_fetch_mid_texture(texture_size);
		struct dtex_cf_texture* up = &c3->t.ONE.s_up_tex;
		struct dtex_cf_texture* down = &c3->t.ONE.d_down_tex;

		int half_sz = texture_size >> 1;

		up->tex = tex;
		up->region.xmin = 0; up->region.xmax = texture_size;
		up->region.ymin = half_sz; up->region.ymax = texture_size;
		up->hash = ds_hash_create(50, 50, 5, ds_string_hash_func, ds_string_equal_func);
		up->tp = dtex_tp_create(texture_size, half_sz, MAX_PRELOAD_COUNT);

		down->tex = tex;
		down->region.xmin = 0; down->region.xmax = texture_size;
		down->region.ymin = 0; down->region.ymax = half_sz;		
		down->hash = ds_hash_create(50, 50, 5, ds_string_hash_func, ds_string_equal_func);
		down->tp = dtex_tp_create(texture_size, half_sz, MAX_PRELOAD_COUNT);
	} else {
		for (int i = 0; i < MULTI_TEX_COUNT; ++i) {
			struct dtex_cf_texture* tex = &c3->t.MULTI.textures[i];
			tex->tex = dtex_res_cache_fetch_mid_texture(texture_size);
			tex->region.xmin = tex->region.ymin = 0;
			tex->region.xmax = tex->region.ymax = texture_size;
			tex->hash = ds_hash_create(50, 50, 5, ds_string_hash_func, ds_string_equal_func);
			tex->tp = dtex_tp_create(texture_size, texture_size, MAX_PRELOAD_COUNT);
		}
	}

	c3->tmp_array = ds_array_create(128, sizeof(struct dtex_cf_node));

	c3->prenode_size = 0;

	return c3;
}

void dtex_c3_release(struct dtex_c3* c3) {
	if (c3->one_tex_mode) {
		dtex_res_cache_return_mid_texture(c3->t.ONE.s_up_tex.tex);

		ds_hash_release(c3->t.ONE.s_up_tex.hash);
		dtex_tp_release(c3->t.ONE.s_up_tex.tp);

		ds_hash_release(c3->t.ONE.d_down_tex.hash);
		dtex_tp_release(c3->t.ONE.d_down_tex.tp);
	} else {
		for (int i = 0; i < MULTI_TEX_COUNT; ++i) {
			struct dtex_cf_texture* tex = &c3->t.MULTI.textures[i];
			dtex_res_cache_return_mid_texture(tex->tex);
			ds_hash_release(tex->hash);
			dtex_tp_release(tex->tp);
		}
	}
	ds_array_release(c3->tmp_array);
	free(c3);
}

void 
dtex_c3_clear(struct dtex_c3* c3) {
	if (c3->one_tex_mode) {
		if (c3->t.ONE.d_down_tex.node_count != 0) {
			dtex_texture_clear_part(c3->t.ONE.d_down_tex.tex, 0, 0, 1, 0.5f);
			dtex_cf_clear_tex_info(&c3->t.ONE.d_down_tex);
		}
	} else {
		for (int i = MULTI_TEX_STATIC_COUNT; i < MULTI_TEX_COUNT; ++i) {
			struct dtex_cf_texture* tex = &c3->t.MULTI.textures[i];
			if (tex->node_count == 0) {
				continue;
			}
			dtex_texture_clear(tex->tex);
			dtex_cf_clear_tex_info(tex);
		}
	}
}

static inline bool
_is_pkg_loaded(struct dtex_c3* c3, const char* pkg_name) {
	struct dtex_cf_node* node = NULL;
	if (c3->one_tex_mode) {
		node = (struct dtex_cf_node*)ds_hash_query(c3->t.ONE.s_up_tex.hash, (void*)pkg_name);
		if (!node) {
			node = (struct dtex_cf_node*)ds_hash_query(c3->t.ONE.d_down_tex.hash, (void*)pkg_name);
		}
	} else {
		for (int i = 0; i < MULTI_TEX_COUNT; ++i) {
			node = (struct dtex_cf_node*)ds_hash_query(c3->t.MULTI.textures[i].hash, (void*)pkg_name);
			if (node) {
				break;
			}
		}
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
		if (c3->prenode_size == MAX_PRELOAD_COUNT) {
			LOGW("%s", "dtex_c3_load preload full");
			return;
		}
		struct dtex_cf_prenode* n = &c3->prenodes[c3->prenode_size++];
		n->pkg = pkg;
		n->tex_idx = i;
		n->scale = scale;
	}
}

static inline bool
_pack_preload_list_with_scale(struct dtex_c3* c3, struct dtex_cf_prenode** pre_list, int pre_sz, float scale) {
	if (c3->one_tex_mode) {
		for (int i = 0; i < pre_sz; ++i) {
			struct dtex_cf_prenode* node = pre_list[i];
			bool succ = false;
			bool is_static = dtex_c3_is_static(node->pkg->c3_stg);
			if (is_static) {
				succ = dtex_cf_pack_prenodes(node, &c3->t.ONE.s_up_tex, scale);
			} else {
				succ = dtex_cf_pack_prenodes(node, &c3->t.ONE.d_down_tex, scale);
			}
			if (!succ) {
				return false;
			}
		}
		return true;
	} else {
		for (int i = 0; i < pre_sz; ++i) {
			struct dtex_cf_prenode* node = pre_list[i];
			bool succ = false;
			bool is_static = dtex_c3_is_static(node->pkg->c3_stg);
			if (is_static) {
				for (int i = 0; i < MULTI_TEX_STATIC_COUNT && !succ; ++i) {
					succ = dtex_cf_pack_prenodes(node, &c3->t.MULTI.textures[i], scale);
				}
			} else {
				for (int i = MULTI_TEX_STATIC_COUNT; i < MULTI_TEX_COUNT && !succ; ++i) {
					succ = dtex_cf_pack_prenodes(node, &c3->t.MULTI.textures[i], scale);
				}				
			}
			if (!succ) {
				return false;
			}
		}
		return true;

	//	// old
	//	// init rect tp
	//	for (int i = 0; i < c3->t.MULTI.tex_size; ++i) {
	//		struct dtex_texture* tex = c3->t.MULTI.textures[i];
	//		assert(tex->type == DTEX_TT_MID);

	//		if (!tex->t.MID.tp) {
	//			tex->t.MID.tp = dtex_tp_create(tex->width, tex->height, c3->prenode_size + 100);
	//		}

	//// 		if (tex->t.MID.tp) {
	//// 			dtex_tp_release(tex->t.MID.tp);
	//// 		}
	//// 		// tp's capacity should larger for later inserting
	//// 		tex->t.MID.tp = dtex_tp_create(tex->width, tex->height, c3->prenode_size + 100);
	//	}

	//	// insert
	//	int first_try_idx = 0;
	//	for (int i = 0; i < pre_sz; ++i) {
	//		struct dtex_cf_prenode* node = pre_list[i];
	//		bool success = false;
	//		for (int j = 0; j < c3->t.MULTI.tex_size; ++j) {
	//			struct dtex_texture* tex = c3->t.MULTI.textures[(first_try_idx+ j) % c3->t.MULTI.tex_size];
	//			success = _pack_preload_node(scale, node, tex, &c3->t.MULTI.index, 0);
	//			if (success) {
	//				first_try_idx = j;
	//				break;
	//			}
	//		}
	//		if (!success) {
	//			return false;
	//		}
	//	}
	//	return true;
	}
}

static inline float
_pack_nodes(struct dtex_c3* c3, struct dtex_cf_prenode** pre_list, int pre_sz, float alloc_scale) {
	qsort((void*)pre_list, pre_sz, sizeof(struct dtex_cf_prenode*), dtex_cf_prenode_size_cmp);

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

// static inline int
// _compare_dr_dst_tex(const void *arg1, const void *arg2) {
// 	struct dtex_cf_node *node1, *node2;
// 
// 	node1 = *((struct dtex_cf_node**)(arg1));
// 	node2 = *((struct dtex_cf_node**)(arg2));
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
_relocate_nodes_cb(struct dtex_import_stream* is, void* ud) {
	// todo: check file type: rrr, b4r
	struct dtex_cf_node* node = (struct dtex_cf_node*)ud;
	struct dtex_texture* tex = node->pkg->textures[node->src_tex_idx];

	bool tex_loaded = false;
	if (tex->id == 0) {
		tex_loaded = false;
		dtex_load_texture_all(is, tex);
	} else {
		tex_loaded = true;
	}

	dtex_shader_scissor(false);
	dtex_draw_begin();
	dtex_cf_relocate_node(tex, node);
	dtex_draw_end();
	dtex_shader_scissor(true);

	if (!tex_loaded) {
		dtex_package_remove_texture_ref(node->pkg, tex);
		dtex_texture_release(tex);
	}

	--node->pkg->c3_loading;
}

static inline void
_relocate_nodes(struct dtex_c3* c3, struct dtex_loader* loader, bool async) {
	// sort all nodes by their texture
	int node_sz = 0;
	struct dtex_cf_node* nodes[MAX_NODE_COUNT];
	if (c3->one_tex_mode) {
		for (int i = 0; i < c3->t.ONE.s_up_tex.node_count; ++i) {
			struct dtex_cf_node* node = &c3->t.ONE.s_up_tex.nodes[i];
			if (!node->finish) {
				assert(node_sz < MAX_NODE_COUNT);
				nodes[node_sz++] = node;
			}
		}
		for (int i = 0; i < c3->t.ONE.d_down_tex.node_count; ++i) {
			struct dtex_cf_node* node = &c3->t.ONE.d_down_tex.nodes[i];
			if (!node->finish) {
				assert(node_sz < MAX_NODE_COUNT);
				nodes[node_sz++] = node;
			}
		}
	} else {
		for (int i = 0; i < MULTI_TEX_COUNT; ++i) {
			struct dtex_cf_texture* tex = &c3->t.MULTI.textures[i];
			for (int j = 0, m = tex->node_count; j < m; ++j) {
				struct dtex_cf_node* node = &tex->nodes[j];
				if (!node->finish) {
					assert(node_sz < MAX_NODE_COUNT);
					nodes[node_sz++] = node;
				}
			}
		}
	}
	qsort((void*)nodes, node_sz, sizeof(struct dtex_cf_node*), dtex_cf_node_pkg_cmp);

	// draw
	struct dtex_package* last_pkg = NULL;
	for (int i = 0; i < node_sz; ++i) {
		struct dtex_cf_node* node = nodes[i];
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
			dtex_shader_scissor(false);
			dtex_draw_begin();
			dtex_cf_relocate_node(ori_tex, node);
			dtex_draw_end();
			dtex_shader_scissor(true);
 			if (!tex_loaded) {
				dtex_package_remove_texture_ref(pkg, ori_tex);
				dtex_texture_release(ori_tex);
 			}
 		}

		last_pkg = pkg;
	}
}

static inline float
_alloc_texture(struct dtex_c3* c3, struct dtex_cf_prenode** pre_list, int pre_sz) {
	// todo
	if (c3->one_tex_mode) {
		return 1;
	} else {
		return 1;
	}

	//float area = 0;
	//for (int i = 0; i < pre_sz; ++i) {
	//	struct dtex_cf_prenode* n = pre_list[i];
	//	struct dtex_texture* tex = n->pkg->textures[n->tex_idx];
	//	int w = tex->width * n->scale,
	//		h = tex->height * n->scale;
	//	area += w * h;
	//}
	//area *= TOT_AREA_SCALE;	

	//for (int i = 0; i < c3->t.MULTI.tex_size; ++i) {
	//	area -= dtex_tp_get_free_space(c3->t.MULTI.textures[i]->t.MID.tp);
	//}
	//if (area <= 0) {
	//	return 1.0f;
	//}

	//while (area > 0) {
	//	if (c3->t.MULTI.tex_size == MULTI_TEX_COUNT) {
	//		fault("c3 texture full.");
	//	}
	//	struct dtex_texture* tex = dtex_res_cache_fetch_mid_texture(c3->tex_edge);
	//	c3->t.MULTI.textures[c3->t.MULTI.tex_size++] = tex;
	//	area -= c3->tex_edge * c3->tex_edge;
	//}

	//// todo: 根据可用内存大小进行缩放
	//return 1.0f;
}

void 
dtex_c3_load_end(struct dtex_c3* c3, struct dtex_loader* loader, bool async) {
	if (c3->prenode_size == 0) {
		return;
	}

	struct dtex_cf_prenode* unique_set[c3->prenode_size];
	int unique_sz = 0;
	dtex_cf_unique_prenodes(c3->prenodes, c3->prenode_size, unique_set, &unique_sz);

	float alloc_scale = _alloc_texture(c3, unique_set, unique_sz);

	/*float scale = */_pack_nodes(c3, unique_set, unique_sz, alloc_scale);

	dtex_shader_scissor(false);
	dtex_draw_begin();
	_relocate_nodes(c3, loader, async);
	dtex_draw_end();
	dtex_shader_scissor(true);

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
// 		struct dtex_cf_node* dr = &hn->n;
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
		ds_hash_query_all(c3->t.ONE.s_up_tex.hash, pkg->name, c3->tmp_array);
		ds_hash_query_all(c3->t.ONE.d_down_tex.hash, pkg->name, c3->tmp_array);
	} else {
		for (int i = 0; i < MULTI_TEX_COUNT; ++i) {
			ds_hash_query_all(c3->t.MULTI.textures[i].hash, pkg->name, c3->tmp_array);
		}
	}

	int sz = ds_array_size(c3->tmp_array);
	for (int i = 0; i < sz; ++i) {
		struct dtex_cf_node* node = *(struct dtex_cf_node**)ds_array_fetch(c3->tmp_array, i);
		textures[node->src_tex_idx] = node->dst_tex->tex;
		regions[node->src_tex_idx]  = &node->dst_rect;
	}
	ds_array_clear(c3->tmp_array);
}

void 
dtex_c3_debug_draw(struct dtex_c3* c3) {
	if (c3->one_tex_mode) {
		dtex_debug_draw(c3->t.ONE.s_up_tex.tex->id, 1);
	} else {
		dtex_debug_draw(c3->t.MULTI.textures[0].tex->id, 1);
		if (MULTI_TEX_COUNT > 1) {
			dtex_debug_draw(c3->t.MULTI.textures[1].tex->id, 2);
		}
	}
}