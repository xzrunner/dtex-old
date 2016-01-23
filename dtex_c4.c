#include "dtex_c4.h"
#include "dtex_cfull.h"
#include "dtex_typedef.h"
#include "dtex_res_cache.h"
#include "dtex_hash.h"
#include "dtex_tp.h"
#include "dtex_package.h"
#include "dtex_log.h"
#include "dtex_texture.h"
#include "dtex_loader.h"
#include "dtex_res_path.h"
#include "dtex_async_loader.h"
#include "dtex_debug.h"
#include "dtex_math.h"
#include "dtex_stream_import.h"
#include "dtex_pvr.h"
#include "dtex_texture_loader.h"
#include "dtex_ej_utility.h"
#include "dtex_gl.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MAX_NODE_COUNT		128
#define MAX_PRELOAD_COUNT	128

struct dtex_c4 {
	int tex_edge;

	struct dtex_cf_texture* textures;
	int max_tex_count;
	int tex_count, used_count;

	struct dtex_cf_prenode* prenodes;
	int prenode_size;

	int process_count;
};

struct dtex_c4* 
dtex_c4_create(int tex_size, int tex_count) {
	size_t textures_sz = sizeof(struct dtex_cf_texture) * tex_count;
	size_t prenodes_sz = sizeof(struct dtex_cf_prenode) * MAX_PRELOAD_COUNT;
	size_t sz = sizeof(struct dtex_c4) + textures_sz + prenodes_sz;
	struct dtex_c4* c4 = (struct dtex_c4*)malloc(sz);
	if (!c4) {
		return NULL;
	}
	memset(c4, 0, sz);

	c4->tex_edge = tex_size;

	c4->max_tex_count = tex_count;
	c4->textures = (struct dtex_cf_texture*)(c4 + 1);
	for (int i = 0; i < tex_count; ++i) {
		struct dtex_cf_texture* tex = &c4->textures[i];
		tex->tex = dtex_texture_create_mid_ref(tex_size);
		tex->ud = dtex_pvr_init_blank(tex_size);
		tex->region.xmin = tex->region.ymin = 0;
		tex->region.xmax = tex->region.ymax = tex_size;
		tex->hash = dtex_hash_create(50, 50, 5, dtex_string_hash_func, dtex_string_equal_func);
		tex->tp = dtex_tp_create(tex_size, tex_size, MAX_PRELOAD_COUNT);
	}
	
	c4->prenodes = (struct dtex_cf_prenode*)((intptr_t)(c4 + 1) + textures_sz);

	return c4;
}

void 
dtex_c4_release(struct dtex_c4* c4) {
	for (int i = 0; i < c4->tex_count; ++i) {
		struct dtex_cf_texture* tex = &c4->textures[i];
		free(tex->ud);
		if (tex->tex->id != 0) {
			dtex_gl_release_texture(tex->tex->id);
		}
		dtex_res_cache_return_mid_texture(tex->tex);
		dtex_hash_release(tex->hash);
		dtex_tp_release(tex->tp);
	}
	free(c4);

	size_t textures_sz = sizeof(struct dtex_cf_texture) * c4->max_tex_count;
	size_t prenodes_sz = sizeof(struct dtex_cf_prenode) * MAX_PRELOAD_COUNT;
	size_t sz = sizeof(struct dtex_c4) + textures_sz + prenodes_sz;
	memset(c4, 0, sz);
}

void 
dtex_c4_clear(struct dtex_c4* c4) {
	for (int i = 0; i < c4->tex_count; ++i) {
		struct dtex_cf_texture* tex = &c4->textures[i];
		if (tex->node_count == 0) {
			continue;
		}
		dtex_texture_clear(tex->tex);
		dtex_cf_clear_tex_info(tex);
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
		n->scale = 1;
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
		for (int j = tex_begin; j <= tex_end && !succ; ++j) {
			if (dtex_cf_pack_prenodes(node, &c4->textures[j], 1)) {
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

static void
_on_load_finished(struct dtex_c4* c4) {
	for (int i = c4->tex_count, n = i + c4->used_count; i < n; ++i) {
		struct dtex_cf_texture* tex = &c4->textures[i];
		tex->tex->id = dtex_load_pvr_tex(tex->ud, tex->tex->width, tex->tex->height, 4);
	}
	c4->tex_count += c4->used_count;	
}

static void
_relocate_nodes_cb(struct dtex_import_stream* is, void* ud) {
	// todo: check file type: rrr, b4r
	struct dtex_cf_node* node = (struct dtex_cf_node*)ud;

	struct dtex_rect* dst_pos = &node->dst_rect;
	assert(IS_4TIMES(dst_pos->xmin) && IS_4TIMES(dst_pos->ymin));

	int format = dtex_import_uint8(is);
	assert(format == DTEX_PVR);
	dtex_import_uint8(is); // pvr_fmt
	int width = dtex_import_uint16(is),
		height = dtex_import_uint16(is);
	dtex_import_uint32(is); // size

	assert(IS_POT(width) && IS_POT(height)
		&& width == dst_pos->xmax - dst_pos->xmin
		&& height == dst_pos->ymax - dst_pos->ymin);

	int grid_x = dst_pos->xmin >> 2,
		grid_y = dst_pos->ymin >> 2;
	int grid_w = width >> 2,
		grid_h = height >> 2;
	grid_y = (node->dst_tex->tex->height >> 2) - grid_y - grid_h;
	const uint8_t* src_data = is->stream;
	uint8_t* dst_data = (uint8_t*)node->dst_tex->ud;
	for (int y = 0; y < grid_h; ++y) {
		for (int x = 0; x < grid_w; ++x) {
			int idx_src = dtex_pvr_get_morton_number(x, y);
			int idx_dst = dtex_pvr_get_morton_number(grid_x + x, grid_y + y);
			assert(idx_dst < node->dst_tex->tex->width * node->dst_tex->tex->height / 16);
			int64_t* src = (int64_t*)src_data + idx_src;
			int64_t* dst = (int64_t*)dst_data + idx_dst;
			memcpy(dst, src, sizeof(int64_t));
		}
	}

	dtex_ej_pkg_traverse(node->pkg->ej_pkg, dtex_cf_relocate_pic, node);

	struct dtex_c4* c4 = (struct dtex_c4*)node->ud;
	if (--c4->process_count == 0) {
		_on_load_finished(c4);
	}
}

static void
_relocate_nodes(struct dtex_c4* c4, struct dtex_loader* loader, bool async, int count) {
	// sort all nodes by their texture
	int node_sz = 0;
	struct dtex_cf_node* nodes[MAX_NODE_COUNT];
	for (int i = c4->tex_count, n = c4->tex_count + count; i < n; ++i) {
		struct dtex_cf_texture* tex = &c4->textures[i];
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
	c4->process_count = node_sz;
	struct dtex_texture* ori_tex = NULL;
	for (int i = 0; i < node_sz; ++i) {
		struct dtex_cf_node* node = nodes[i];
		node->ud = c4;
		node->finish = true;
		struct dtex_package* pkg = node->pkg;
		ori_tex = pkg->textures[node->src_tex_idx];
		assert(ori_tex && ori_tex->type == DTEX_TT_RAW);

		int pkg_idx = dtex_package_texture_idx(pkg, ori_tex);
		assert(pkg_idx != -1);
		const char* path = dtex_get_img_filepath(pkg->rp, pkg_idx, pkg->LOD);
		assert(ori_tex->id == 0);
		if (async) {
			dtex_async_load_file(path, _relocate_nodes_cb, node, "c4");
		} else {
			dtex_load_file(path, _relocate_nodes_cb, node);
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

	c4->used_count = _pack_nodes(c4, unique_set, unique_sz);
	_relocate_nodes(c4, loader, async, c4->used_count);
	
	c4->prenode_size = 0;
}

void 
dtex_c4_debug_draw(struct dtex_c4* c4) {
	dtex_debug_draw(c4->textures[0].tex->id, 3);
}