#include "dtex_c4.h"
#include "dtex_cfull.h"
#include "dtex_typedef.h"
#include "dtex_res_cache.h"
#include "dtex_tp.h"
#include "dtex_package.h"
#include "dtex_texture.h"
#include "dtex_loader.h"
#include "dtex_res_path.h"
#include "dtex_async_loader.h"
#include "dtex_debug.h"
#include "dtex_math.h"
#include "dtex_stream_import.h"
#include "dtex_pvr.h"
#include "dtex_etc2.h"
#include "dtex_png.h"
#include "dtex_texture_loader.h"
#include "dtex_ej_utility.h"
#include "dtex_gl.h"
#include "dtex_bitmap.h"
#include "dtex_hard_res.h"

// todo
#include "dtex_facade.h"

#include <ds_hash.h>
#include <logger.h>

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MAX_NODE_COUNT		512
#define MAX_PRELOAD_COUNT	512

struct dtex_c4 {
	int tex_edge;

	struct dtex_cf_texture* textures;
	int max_tex_count;
	int tex_count, used_count;

	struct dtex_cf_prenode* prenodes;
	int prenode_size;

	int process_count;
};

static int
_get_tex_type() {
#if defined(__APPLE__) && !defined(__MACOSX)
	return DTEX_PVR;
#elif defined(__ANDROID__)
	return DTEX_ETC2;
#else
	return DTEX_PNG8;
#endif
}

struct dtex_c4* 
dtex_c4_create(int tex_size, int tex_count) {
	int max_sz = dtex_max_texture_size();
	while (tex_size > max_sz) {
		tex_size = tex_size >> 1;
		tex_count = tex_count << 2;
	}

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
		tex->tex = dtex_texture_create_mid_ref(tex_size, tex_size);
		tex->ud = NULL;
		tex->region.xmin = tex->region.ymin = 0;
		tex->region.xmax = tex->region.ymax = tex_size;
		tex->hash = ds_hash_create(50, 50, 5, ds_string_hash_func, ds_string_equal_func);
		tex->tp = dtex_tp_create(tex_size, tex_size, MAX_PRELOAD_COUNT);
	}
	
	c4->prenodes = (struct dtex_cf_prenode*)((intptr_t)(c4 + 1) + textures_sz);

	return c4;
}

void 
dtex_c4_release(struct dtex_c4* c4) {
	for (int i = 0; i < c4->max_tex_count; ++i) {
		struct dtex_cf_texture* tex = &c4->textures[i];

		if (tex->ud) {
			uint8_t* pixels = (uint8_t*)(tex->ud);
			free(pixels);
			tex->ud = NULL;
		}

		if (tex->tex->id != 0) {
			dtex_gl_release_texture(tex->tex->id);
		}

		ds_hash_release(tex->hash);
		dtex_tp_release(tex->tp);
	}
	free(c4);
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
		if (ds_hash_query(c4->textures[i].hash, pkg->name)) {
			return;
		}
	}
	
	for (int i = 0; i < pkg->texture_count; ++i) {
		struct dtex_texture* tex = pkg->textures[i];
		if (_get_tex_type() != DTEX_PNG8 && tex->t.RAW.format != _get_tex_type()) {
			dtexf_load_texture(pkg, i);
			continue;
		}
		if (c4->prenode_size == MAX_PRELOAD_COUNT) {
			LOGW("%s", "dtex_c4_load preload full");
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
			LOGW("%s", "dtex_c4_load_end node insert fail.");
			dtexf_load_texture(node->pkg, node->tex_idx);
		}
	}
	return tex_max - tex_begin + 1;
}

static void
_on_load_finished(struct dtex_c4* c4) {
	int tex_type = _get_tex_type();
	for (int i = c4->tex_count, n = i + c4->used_count; i < n; ++i) {
		struct dtex_cf_texture* tex = &c4->textures[i];
		uint8_t* pixels = (uint8_t*)(tex->ud);
		if (tex_type == DTEX_PVR) {
			tex->tex->id = dtex_load_pvr_tex(pixels, tex->tex->width, tex->tex->height, 4);
		} else if (tex_type == DTEX_ETC2) {
			tex->tex->id = dtex_load_etc2_tex(pixels, tex->tex->width, tex->tex->height);
		} else if (tex_type == DTEX_PNG8) {
			tex->tex->id = dtex_gl_create_texture(DTEX_TF_RGBA8, tex->tex->width, tex->tex->height, pixels, 0, 0);
		}
		free(pixels);
		tex->ud = NULL;
	}
	c4->tex_count += c4->used_count;	
}

static void
_load_pvr_header(struct dtex_import_stream* is, int* w, int* h) {
	dtex_import_uint8(is); // pvr_fmt
	*w = dtex_import_uint16(is);
	*h = dtex_import_uint16(is);
	dtex_import_uint32(is); // size
}

static void
_load_part_pvr(struct dtex_import_stream* is, struct dtex_cf_node* node) {
	struct dtex_rect* dst_pos = &node->dst_rect;
	assert(IS_4TIMES(dst_pos->xmin) && IS_4TIMES(dst_pos->ymin));

	if (!node->dst_tex->ud) {
		node->dst_tex->ud = dtex_pvr_init_blank(node->dst_tex->tex->width);
	}

	int w, h;
	_load_pvr_header(is, &w, &h);
	assert(IS_POT(w) && IS_POT(h)
		&& w == dst_pos->xmax - dst_pos->xmin
		&& h == dst_pos->ymax - dst_pos->ymin);

	int grid_x = dst_pos->xmin >> 2,
		grid_y = dst_pos->ymin >> 2;
	int grid_w = w >> 2,
		grid_h = h >> 2;
	const uint8_t* src_data = (const uint8_t*)(is->stream);
	uint8_t* dst_data = (uint8_t*)(node->dst_tex->ud);
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
}

static void
_load_etc2_header(struct dtex_import_stream* is, int* w, int* h) {
	*w = dtex_import_uint16(is);
	*h = dtex_import_uint16(is);
}

static void
_load_part_etc2(struct dtex_import_stream* is, struct dtex_cf_node* node) {
	struct dtex_rect* dst_pos = &node->dst_rect;
	assert(IS_4TIMES(dst_pos->xmin) && IS_4TIMES(dst_pos->ymin));

	if (!node->dst_tex->ud) {
		node->dst_tex->ud = dtex_etc2_init_blank(node->dst_tex->tex->width);
	}

	int w, h;
	_load_etc2_header(is, &w, &h);
	assert(IS_POT(w) && IS_POT(h)
		&& w == dst_pos->xmax - dst_pos->xmin
		&& h == dst_pos->ymax - dst_pos->ymin);

	int grid_x = dst_pos->xmin >> 2,
		grid_y = dst_pos->ymin >> 2;
	int grid_w = w >> 2,
		grid_h = h >> 2;
	const uint8_t* src_data = (const uint8_t*)(is->stream);
	uint8_t* dst_data = (uint8_t*)(node->dst_tex->ud);
	const int grid_sz = sizeof(uint8_t) * 8 * 2;
	const int large_grid_w = node->dst_tex->tex->width >> 2;
	for (int y = 0; y < grid_h; ++y) {
		for (int x = 0; x < grid_w; ++x) {
			int idx_src = x + grid_w * y;
			int idx_dst = grid_x + x + large_grid_w * (grid_y + y);
			assert(idx_dst < node->dst_tex->tex->width * node->dst_tex->tex->height / 16);
			memcpy(dst_data + idx_dst * grid_sz, src_data + idx_src * grid_sz, grid_sz);
		}
	}
}

static void
_load_part_png8_data(int w, int h, const uint8_t* pixels, struct dtex_cf_node* node) {
	struct dtex_rect* dst_pos = &node->dst_rect;
	assert(w == dst_pos->xmax - dst_pos->xmin
		&& h == dst_pos->ymax - dst_pos->ymin);

	const uint8_t* src_data = pixels;
	uint8_t* dst_data = (uint8_t*)(node->dst_tex->ud);
	const int large_w = node->dst_tex->tex->width;
	for (int y = 0; y < h; ++y) {
		int idx_src = w * y;
		int idx_dst = dst_pos->xmin + large_w * (dst_pos->ymin + y);
		memcpy(dst_data + idx_dst * 4, src_data + idx_src * 4, 4 * w);
	}
}

static void
_load_part_png8(struct dtex_import_stream* is, struct dtex_cf_node* node) {
 	int w = dtex_import_uint16(is),
 		h = dtex_import_uint16(is);
	_load_part_png8_data(w, h, (const uint8_t*)(is->stream), node);
}

static void
_load_part_pvr_as_png(struct dtex_import_stream* is, struct dtex_cf_node* node) {
	int w, h;
	_load_pvr_header(is, &w, &h);

	const uint8_t* pixels = (const uint8_t*)(is->stream);
	uint8_t* uncompressed = dtex_pvr_decode(pixels, w, h);
	dtex_bmp_revert_y((uint32_t*)uncompressed, w, h);
	_load_part_png8_data(w, h, uncompressed, node);
	free(uncompressed);
}

static void
_load_part_etc2_as_png(struct dtex_import_stream* is, struct dtex_cf_node* node) {
	int w, h;
	_load_etc2_header(is, &w, &h);

	const uint8_t* pixels = (const uint8_t*)(is->stream);
	uint8_t* uncompressed = dtex_etc2_decode(pixels, w, h, ETC2PACKAGE_RGBA_NO_MIPMAPS);
	_load_part_png8_data(w, h, uncompressed, node);
	free(uncompressed);
}

static void
_relocate_nodes_cb(struct dtex_import_stream* is, void* ud) {
	// todo: check file type: rrr, b4r

	struct dtex_cf_node* node = (struct dtex_cf_node*)ud;

	int format = dtex_import_uint8(is);
	int tex_type = _get_tex_type();
	switch (tex_type)
	{
	case DTEX_PVR:
		assert(format == DTEX_PVR);
		_load_part_pvr(is, node);
		break;
	case DTEX_ETC2:
		assert(format == DTEX_ETC2);
		_load_part_etc2(is, node);
		break;
	case DTEX_PNG8:
		if (!node->dst_tex->ud) {
			node->dst_tex->ud = dtex_bmp_init_blank(node->dst_tex->tex->width);
		}
		if (format == DTEX_PNG8) {
			_load_part_png8(is, node);
		} else if (format == DTEX_PVR) {
			_load_part_pvr_as_png(is, node);
		} else if (format == DTEX_ETC2) {
			_load_part_etc2_as_png(is, node);
		} else {
			assert(0);
		}
		break;
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
		LOGW("%s", "dtex_c4_load_end tex full.");
		return;
	}

	if (c4->prenode_size == 0) {
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
	if (c4->tex_count > 0) {
		dtex_debug_draw(c4->textures[0].tex->id, 2);
	}
	if (c4->tex_count > 1) {
		dtex_debug_draw(c4->textures[1].tex->id, 3);
	}
}