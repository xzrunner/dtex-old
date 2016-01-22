#include "dtex_c4.h"
#include "dtex_cfull.h"
#include "dtex_typedef.h"
#include "dtex_res_cache.h"
#include "dtex_hash.h"
#include "dtex_tp.h"
#include "dtex_package.h"
#include "dtex_log.h"
#include "dtex_texture.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MAX_NODE_COUNT		128
#define MAX_PRELOAD_COUNT	128

struct c4_texture {
	struct dtex_texture* texture;

	struct dtex_cf_node nodes[MAX_NODE_COUNT];
	int node_count;

	struct dtex_hash* hash;
	struct dtex_tp* tp;
};

struct dtex_c4 {
	int tex_edge;

	struct c4_texture* textures;
	int max_tex_count;
	int tex_count;

	struct dtex_cf_prenode* prenodes;
	int prenode_size;
};

struct dtex_c4* 
dtex_c4_create(int tex_size, int tex_count) {
	size_t textures_sz = sizeof(struct c4_texture) * tex_count;
	size_t prenodes_sz = sizeof(struct dtex_cf_prenode) * MAX_PRELOAD_COUNT;
	size_t sz = sizeof(struct dtex_c4) + textures_sz + prenodes_sz;
	struct dtex_c4* c4 = (struct dtex_c4*)malloc(sz);
	if (!c4) {
		return NULL;
	}
	memset(c4, 0, sz);

	c4->tex_edge = tex_size;

	c4->max_tex_count = tex_count;
	c4->textures = (struct c4_texture*)(c4 + 1);
	
	c4->prenodes = (struct dtex_cf_prenode*)((intptr_t)(c4 + 1) + textures_sz);

	return c4;
}

void 
dtex_c4_release(struct dtex_c4* c4) {
	for (int i = 0; i < c4->tex_count; ++i) {
		struct c4_texture* tex = &c4->textures[i];

		// todo release tex
//		dtex_res_cache_return_mid_texture(tex->texture);

		dtex_hash_release(tex->hash);
		dtex_tp_release(tex->tp);
	}
	free(c4);

	size_t textures_sz = sizeof(struct c4_texture) * c4->max_tex_count;
	size_t prenodes_sz = sizeof(struct dtex_cf_prenode) * MAX_PRELOAD_COUNT;
	size_t sz = sizeof(struct dtex_c4) + textures_sz + prenodes_sz;
	memset(c4, 0, sz);
}

void 
dtex_c4_clear(struct dtex_c4* c4) {
	for (int i = 0; i < c4->tex_count; ++i) {
		struct c4_texture* tex = &c4->textures[i];

		// todo clear tex
		// dtex_res_cache_return_mid_texture(tex->texture);

		dtex_hash_clear(tex->hash);
		dtex_tp_clear(tex->tp);
	}
}

void 
dtex_c4_load(struct dtex_c4* c4, struct dtex_package* pkg) {
	for (int i = 0; i < c4->max_tex_count; ++i) {
		if (dtex_hash_query(c4->textures[i].hash, pkg->name)) {
			return;
		}
	}
	
	for (int i = 0; i < pkg->texture_count; ++i) {
		if (c4->prenode_size == MAX_PRELOAD_COUNT) {
			dtex_warning("dtex_c4_load preload full");
			return;
		}
		struct dtex_cf_prenode* n = &c4->prenodes[c4->prenode_size++];
		n->pkg = pkg;
		n->tex_idx = i;
	}
}

static int
_pack_nodes(struct dtex_c4* c4, struct dtex_cf_prenode** pre_list, int pre_sz) {
	int tex_max = 0;
	int tex_begin = c4->tex_count, tex_end = c4->max_tex_count - 1;
	qsort((void*)pre_list, pre_sz, sizeof(struct dtex_cf_prenode*), dtex_cf_prenode_size_cmp);
	for (int i = 0; i < pre_sz; ++i) {
		struct dtex_cf_prenode* node = pre_list[i];
		bool succ = false;
		for (int j = tex_begin; j < tex_end && !succ; ++j) {
			if (dtex_cf_pack_prenodes(node, &c4->textures[j])) {
				if (j > tex_max) {
					tex_max = j;
				}
				succ = true;
			}
		}
		if (!succ) {
			dtex_warning("+++++++++++++++++  dtex_c4_load_end node insert fail.");
		}
	}
	return tex_max - tex_begin + 1;
}

static inline void
_relocate_node(struct dtex_texture* src, struct c4_node* dst) {
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
}

static void
_relocate_nodes(struct dtex_c4* c4, struct dtex_loader* loader, bool async, int count) {
	// sort all nodes by their texture
	int node_sz;
	struct dtex_cf_node* nodes[MAX_NODE_COUNT];
	for (int i = c4->tex_count, n = c4->tex_count + count; i < n; ++i) {
		struct c4_texture* tex = &c4->textures[i];
		for (int j = 0; j < tex->node_count; ++j) {
			struct dtex_cf_node* node = &tex->nodes[j];
			if (!node->finish) {
				assert(node_sz < MAX_NODE_COUNT);
				nodes[node_sz++] = node;
			}
		}
	}
	qsort((void*)nodes, node_sz, sizeof(struct dtex_cf_node*), dtex_cf_node_pkg_cmp);

	// draw
	bool tex_loaded = false;
	struct dtex_texture* ori_tex = NULL;
	for (int i = 0; i < node_sz; ++i) {
		struct dtex_cf_node* node = nodes[i];
		node->finish = true;
		struct dtex_package* pkg = node->pkg;
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
			++pkg->c4_loading;
			const char* path = dtex_get_img_filepath(pkg->rp, pkg_idx, pkg->LOD);
			dtex_async_load_file(path, _relocate_nodes_cb, node, "c4");
		}

		if (!async) {
			_relocate_node(ori_tex, node);
			if (!tex_loaded) {
				dtex_package_remove_texture_ref(pkg, ori_tex);
				dtex_texture_release(ori_tex);
			}
		}
	}
}

void 
dtex_c4_load_end(struct dtex_c4* c4, struct dtex_loader* loader, bool async) {
	if (c4->tex_count >= c4->max_tex_count) {
		dtex_warning("+++++++++++++++++  dtex_c4_load_end tex full.");
		return;
	}

	struct dtex_cf_prenode* unique_set[c4->prenode_size];
	int unique_sz = 0;
	dtex_cf_unique_prenodes(c4->prenodes, c4->prenode_size, unique_set, &unique_sz);

	int used_count = _pack_nodes(c4, unique_set, unique_sz);
	_relocate_nodes(c4, loader, async, used_count);
	
	c4->prenode_size = 0;
}
