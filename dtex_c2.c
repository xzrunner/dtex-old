#include "dtex_c2.h"
#include "dtex_tp.h"
#include "dtex_loader.h"
#include "dtex_rrp.h"
#include "dtex_package.h"
#include "dtex_ej_utility.h"
#include "dtex_relocation.h"
#include "dtex_texture.h"
#include "dtex_res_cache.h"
#include "dtex_c2_strategy.h"
#include "dtex_debug.h"
#include "dtex_render.h"
#include "dtex_cg.h"
#include "dtex_gl.h"
#include "dtex_shader.h"
#include "dtex_hard_res.h"

#include "ejoy2d.h"

#include <ds_hash.h>
#include <logger.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MAX_TEX_COUNT		4

#define MAX_NODE_COUNT		4096*2
#define MAX_PRELOAD_COUNT	4096*2

#define DEFAULT_PKG_ID		4096
#define DEFAULT_QUAD_IDX	0

#define PADDING				0
#define EXTRUDE				1

#define CG_QUAD				2

#define MAX_QUAD_SIZE		768

static int SRC_EXTRUDE = 0;

struct hash_key {
	int pkg_id;
	int spr_id;
	int quad_idx;
};

struct c2_node {
	// dst info
	struct dtex_texture* dst_tex;
	struct dtex_tp_pos* dst_pos;
	float dst_vb[8];

	// draw ori to dst
	float trans_vb[16];

	// hash
	struct hash_key hk;
};

enum C2_PRENODE_TYPE {
	C2_PT_SPR = 0,
	C2_PT_TEX,
	C2_PT_USERDATA,
};

struct c2_prenode {
	int type;
	union {
		struct {
			struct dtex_package* pkg;
			struct ej_pack_quad* ej_quad;
			struct dtex_texture* ori_tex;
			int spr_id;
			int quad_idx;

			struct dtex_rect rect;
		} SPR;

		struct {
			int key;
			int id;
			int w, h;
		} TEX;

		struct {
			uint32_t userdata;
			int tex_id;
			int tex_w, tex_h;
			struct dtex_rect rect;
		} UD;
	} t;
};

struct tp_index {
	struct c2_node nodes[MAX_NODE_COUNT];
	int node_count;

	struct ds_hash* hash;

	struct dtex_tp* tp;

	bool is_static;
};

// todo hash prenodes
struct dtex_c2 {
	int loadable;

	int static_quad;

	bool one_tex_mode;
	union {
		struct {
			struct dtex_texture* texture;
			struct tp_index index[4];	// 0 1
										// 2 3
			int clear_idx;
			struct dtex_cg* cg;
		} ONE;

		struct {
 			struct dtex_texture* textures[MAX_TEX_COUNT];
 			int tex_size;
			struct tp_index index;
		} MULTI;
	} t;

	int prenode_size;	
	struct c2_prenode prenodes[1];
};

static inline unsigned int 
_hash_func(int hash_sz, void* key) {
	struct hash_key* hk = (struct hash_key*)key;
	uint32_t hash = (hk->pkg_id * 51439) ^ (hk->spr_id * 97) ^ hk->quad_idx;
	return hash % hash_sz;
}

static inline bool 
_equal_func(void* key0, void* key1) {
	struct hash_key* hk0 = (struct hash_key*)key0;
	struct hash_key* hk1 = (struct hash_key*)key1;
	return hk0->pkg_id == hk1->pkg_id
		&& hk0->spr_id == hk1->spr_id
		&& hk0->quad_idx == hk1->quad_idx;
}

static inline void
_clear_tp_index(struct tp_index* index) {
	index->node_count = 0;

	ds_hash_clear(index->hash);

	if (index->tp) {
		dtex_tp_clear(index->tp);
	}
}

struct dtex_c2* 
dtex_c2_create(int width, int height, bool one_tex_mode, int static_count, bool open_cg, int src_extrude) {
	int max_sz = dtex_max_texture_size();
	if (width > max_sz) {
		width = max_sz;
	}
	if (height > max_sz) {
		height = max_sz;
	}

	SRC_EXTRUDE = src_extrude;

	size_t sz = sizeof(struct dtex_c2) + sizeof(struct c2_prenode) * MAX_PRELOAD_COUNT;
	struct dtex_c2* c2 = (struct dtex_c2*)malloc(sz);
	if (!c2) {
		return NULL;
	}
	memset(c2, 0, sz);

	c2->one_tex_mode = one_tex_mode;
	if (one_tex_mode) {
		c2->t.ONE.texture = dtex_res_cache_fetch_mid_texture(width, height);
		c2->t.ONE.clear_idx = static_count;
		int hw = width >> 1;
		int hh = height >> 1;
		for (int i = 0; i < 4; ++i) {
			struct tp_index* index = &c2->t.ONE.index[i];
			index->hash = ds_hash_create(1000, 2000, 0.5f, _hash_func, _equal_func);
			index->tp = dtex_tp_create(hw, hh, MAX_PRELOAD_COUNT / 4);
			index->is_static = i < static_count;
		}
		c2->t.ONE.cg = NULL;
		if (open_cg) {
			c2->t.ONE.cg = dtex_cg_create(c2->t.ONE.index[CG_QUAD].tp, c2->t.ONE.texture);
		}
	} else {
		struct dtex_texture* tex = dtex_res_cache_fetch_mid_texture(width, height);
		tex->t.MID.tp = dtex_tp_create(tex->width, tex->height, MAX_PRELOAD_COUNT);
		c2->t.MULTI.textures[c2->t.MULTI.tex_size++] = tex;

		c2->t.MULTI.index.hash = ds_hash_create(1000, 2000, 0.5f, _hash_func, _equal_func);
		c2->t.MULTI.index.tp = NULL;
	}

	c2->prenode_size = 0;

	c2->static_quad = -1;

	return c2;
}

void 
dtex_c2_release(struct dtex_c2* c2) {
	if (c2->one_tex_mode) {
		dtex_res_cache_return_mid_texture(c2->t.ONE.texture);
		for (int i = 0; i < 4; ++i) {
			ds_hash_release(c2->t.ONE.index[i].hash);
			dtex_tp_release(c2->t.ONE.index[i].tp);
		}
		if (c2->t.ONE.cg) {
			dtex_cg_release(c2->t.ONE.cg);
		}
	} else {
		for (int i = 0; i < c2->t.MULTI.tex_size; ++i) {
			dtex_res_cache_return_mid_texture(c2->t.MULTI.textures[i]);
		}
		ds_hash_release(c2->t.MULTI.index.hash);
	}
	free(c2);
}

static inline void
_clear(struct dtex_c2* c2, struct dtex_loader* loader) {
	LOGW("%s", "c2 clear");

	c2->loadable = 0;

	if (c2->one_tex_mode) {
		int idx = c2->t.ONE.clear_idx;

		float x = 0, y = 0;
		if (idx == 0 || idx == 1) {
			y = 0.5f;
		}
		if (idx == 3 || idx == 1) {
			x = 0.5f;
		}

		if (idx == CG_QUAD) {
			dtex_cg_clear(c2->t.ONE.cg);
		}

		dtex_shader_flush();
		dtex_texture_clear_part(c2->t.ONE.texture, x, y, x+0.5f, y+0.5f);
		_clear_tp_index(&c2->t.ONE.index[idx]);
		c2->t.ONE.clear_idx = (idx + 1) % 4;
	} else {
		for (int i = 0; i < c2->t.MULTI.tex_size; ++i) {
			struct dtex_texture* tex = c2->t.MULTI.textures[i];
			assert(tex->type == DTEX_TT_MID);
			dtex_texture_clear(tex);
			dtex_tp_clear(tex->t.MID.tp);
		}
		_clear_tp_index(&c2->t.MULTI.index);
	}

	c2->prenode_size = 0;

	dtex_package_traverse(loader, dtex_c2_strategy_clear);
}

void 
dtex_c2_clear(struct dtex_c2* c2) {
	if (!c2->one_tex_mode) {
		return;
	}

	dtex_texture_clear(c2->t.ONE.texture);
	for (int i = 0; i < 4; ++i) {
		_clear_tp_index(&c2->t.ONE.index[i]);
	}
	dtex_cg_clear(c2->t.ONE.cg);
}

void 
dtex_c2_clear_from_cg(struct dtex_c2* c2, struct dtex_loader* loader)
{
	if (!c2->one_tex_mode) {
		return;
	}

	c2->t.ONE.clear_idx = CG_QUAD;
	_clear(c2, loader);
}

int 
dtex_c2_get_cg_quad() {
	return CG_QUAD;
}

void 
dtex_c2_set_static_quad(struct dtex_c2* c2, int quad) {
	assert(quad >= -1 && quad <= 3);
	c2->static_quad = quad;
}

void 
dtex_c2_load_begin(struct dtex_c2* c2) {
	c2->loadable++;
}

struct preload_picture_params {
	struct dtex_c2* c2;
	struct dtex_package* pkg;
};

static inline void 
_get_texcoords_region(uint16_t* texcoords, struct dtex_rect* region) {
	region->xmin = region->ymin = INT16_MAX;
	region->xmax = region->ymax = 0;
	for (int i = 0; i < 4; ++i) {
		if (texcoords[i*2]   < region->xmin) region->xmin = texcoords[i*2];
		if (texcoords[i*2]   > region->xmax) region->xmax = texcoords[i*2];
		if (texcoords[i*2+1] < region->ymin) region->ymin = texcoords[i*2+1];
		if (texcoords[i*2+1] > region->ymax) region->ymax = texcoords[i*2+1];
	}
}

static inline void
_preload_picture(int pic_id, struct ej_pack_picture* ej_pic, void* ud) {
	struct preload_picture_params* params = (struct preload_picture_params*)ud;

	if (params->c2->loadable == 0) {
		return;
	}

	if (params->c2->prenode_size >= MAX_PRELOAD_COUNT - 1) {
		LOGW("%s", "c2 prenode full.");
		return;
	}

	for (int i = 0; i < ej_pic->n; ++i) {
		if (params->c2->prenode_size == MAX_PRELOAD_COUNT - 1) {
			break;
		}
		struct ej_pack_quad* ej_q = &ej_pic->rect[i];
		struct dtex_rect rect;
		_get_texcoords_region(ej_q->texture_coord, &rect);
		if (rect.xmax - rect.xmin > MAX_QUAD_SIZE ||
			rect.ymax - rect.ymin > MAX_QUAD_SIZE) {
			continue;
		}

		struct c2_prenode* pn = &params->c2->prenodes[params->c2->prenode_size++];
		pn->type = C2_PT_SPR;
		pn->t.SPR.pkg = params->pkg;
		pn->t.SPR.ej_quad = ej_q;
		if (ej_q->texid < QUAD_TEXID_IN_PKG_MAX) {
			pn->t.SPR.ori_tex = params->pkg->textures[ej_q->texid];
		} else {
			pn->t.SPR.ori_tex = dtex_texture_fetch(ej_q->texid);
		}
		pn->t.SPR.spr_id = pic_id;
		pn->t.SPR.quad_idx = i;
		pn->t.SPR.rect = rect;
	}
}

void 
dtex_c2_load_spr(struct dtex_c2* c2, struct dtex_package* pkg, int spr_id) {
	assert(spr_id >= 0);

	if (c2->loadable == 0) {
		return;
	}

	if (c2->prenode_size >= MAX_PRELOAD_COUNT - 1) {
		LOGW("%s", "c2 prenode full.");
		return;
	}

	struct preload_picture_params params;
	params.c2 = c2;
	params.pkg = pkg;
	dtex_ej_spr_traverse(pkg->ej_pkg, spr_id, _preload_picture, &params);
}

void 
dtex_c2_load_tex(struct dtex_c2* c2, int tex_id, int tex_width, int tex_height, int key) {
	if (tex_id <= 0 || tex_width <= 0 || tex_height <= 0) {
		return;
	}
	if (c2->prenode_size == MAX_PRELOAD_COUNT - 1) {
		return;
	}

	struct c2_prenode* pn = &c2->prenodes[c2->prenode_size++];
	pn->type = C2_PT_TEX;
	pn->t.TEX.key = key;
	pn->t.TEX.id = tex_id;
	pn->t.TEX.w = tex_width;
	pn->t.TEX.h = tex_height;
}

void 
dtex_c2_load_userdata(struct dtex_c2* c2, int tex_id, int tex_width, int tex_height, const struct dtex_rect* rect, uint32_t userdata) {
	if (tex_id <= 0 || tex_width <= 0 || tex_height <= 0) {
		return;
	}
	if (c2->prenode_size == MAX_PRELOAD_COUNT - 1) {
		return;
	}

	struct c2_prenode* pn = &c2->prenodes[c2->prenode_size++];
	pn->type = C2_PT_USERDATA;
	pn->t.UD.userdata = userdata;
	pn->t.UD.tex_id = tex_id;
	pn->t.UD.tex_w = tex_width;
	pn->t.UD.tex_h = tex_height;
	pn->t.UD.rect = *rect;
}

static inline int 
_compare_unique(const void *arg1, const void *arg2) {
	struct c2_prenode *node1, *node2;

	node1 = *((struct c2_prenode**)(arg1));
	node2 = *((struct c2_prenode**)(arg2));

	if (node1->type == C2_PT_SPR && node2->type == C2_PT_SPR) {
		if(node1->t.SPR.rect.xmin < node2->t.SPR.rect.xmin) return -1;
		if(node1->t.SPR.rect.xmin > node2->t.SPR.rect.xmin) return 1;

		if(node1->t.SPR.rect.xmax < node2->t.SPR.rect.xmax) return -1;
		if(node1->t.SPR.rect.xmax > node2->t.SPR.rect.xmax) return 1;

		if(node1->t.SPR.rect.ymin < node2->t.SPR.rect.ymin) return -1;
		if(node1->t.SPR.rect.ymin > node2->t.SPR.rect.ymin) return 1;

		if(node1->t.SPR.rect.ymax < node2->t.SPR.rect.ymax) return -1;
		if(node1->t.SPR.rect.ymax > node2->t.SPR.rect.ymax) return 1;

		if(node1->t.SPR.pkg->id < node2->t.SPR.pkg->id) return -1;
		if(node1->t.SPR.pkg->id > node2->t.SPR.pkg->id) return 1;

		if(node1->t.SPR.spr_id < node2->t.SPR.spr_id) return -1;
		if(node1->t.SPR.spr_id > node2->t.SPR.spr_id) return 1;

		if(node1->t.SPR.quad_idx < node2->t.SPR.quad_idx) return -1;
		if(node1->t.SPR.quad_idx > node2->t.SPR.quad_idx) return 1;

		return 0;
	} else if (node1->type == C2_PT_TEX && node2->type == C2_PT_TEX) {
		return node1->t.TEX.key < node2->t.TEX.key ? -1 : 1;
	} else if (node1->type == C2_PT_USERDATA && node2->type == C2_PT_USERDATA) {
		return node1->t.UD.userdata < node2->t.UD.userdata ? -1 : 1;
	} else {
		return node1->type == C2_PT_SPR ? -1 : 1;
	}	
}

static inline void
_get_unique_prenodes(struct dtex_c2* c2, struct c2_prenode** ret_set, int* ret_sz) {
	for (int i = 0; i < c2->prenode_size; ++i) {
		ret_set[i] = &c2->prenodes[i];
	}
	qsort((void*)ret_set, c2->prenode_size, sizeof(struct c2_prenode*), _compare_unique);

	struct c2_prenode* unique[MAX_PRELOAD_COUNT];
	unique[0] = ret_set[0];
	int unique_size = 1;
	for (int i = 1; i < c2->prenode_size; ++i) {
		struct c2_prenode* last = unique[unique_size-1];
		struct c2_prenode* curr = ret_set[i];
		if (_compare_unique(&curr, &last) == 0) {
			;
		} else {
			unique[unique_size++] = curr;
		}
	}
	memcpy(ret_set, unique, sizeof(struct c2_prenode*) * unique_size);
	*ret_sz = unique_size;
}

static inline int
_compare_max_edge(const void *arg1, const void *arg2) {
	struct c2_prenode *node1, *node2;

	node1 = *((struct c2_prenode**)(arg1));
	node2 = *((struct c2_prenode**)(arg2));

	int16_t w1, h1, w2, h2;
	if (node1->type == C2_PT_SPR) {
		w1 = node1->t.SPR.rect.xmax - node1->t.SPR.rect.xmin;
		h1 = node1->t.SPR.rect.ymax - node1->t.SPR.rect.ymin;
	} else if (node1->type == C2_PT_TEX) {
		w1 = node1->t.TEX.w;
		h1 = node1->t.TEX.h;
	} else {
		assert(node1->type == C2_PT_USERDATA);
		w1 = node1->t.UD.rect.xmax - node1->t.UD.rect.xmin;
		h1 = node1->t.UD.rect.ymax - node1->t.UD.rect.ymin;
	}
	if (node2->type == C2_PT_SPR) {
		w2 = node2->t.SPR.rect.xmax - node2->t.SPR.rect.xmin;
		h2 = node2->t.SPR.rect.ymax - node2->t.SPR.rect.ymin;
	} else if (node1->type == C2_PT_TEX) {
		w2 = node2->t.TEX.w;
		h2 = node2->t.TEX.h;
	} else {
		assert(node2->type == C2_PT_USERDATA);
		w2 = node2->t.UD.rect.xmax - node2->t.UD.rect.xmin;
		h2 = node2->t.UD.rect.ymax - node2->t.UD.rect.ymin;
	}

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

static inline struct c2_node*
_query_node(struct dtex_c2* c2, int pkg_id, int spr_id, int quad_idx, struct tp_index** tp) {
	struct hash_key hk;
	hk.pkg_id = pkg_id;
	hk.spr_id = spr_id;
	hk.quad_idx = quad_idx;
	if (c2->one_tex_mode) {
		for (int i = 0; i < 4; ++i) {
			struct c2_node* ret = (struct c2_node*)ds_hash_query(c2->t.ONE.index[i].hash, &hk);
			if (ret) {
				*tp = &c2->t.ONE.index[i];
				return ret;
			}
		}
	} else {
		*tp = &c2->t.MULTI.index;
		return (struct c2_node*)ds_hash_query(c2->t.MULTI.index.hash, &hk);
	}
	*tp = NULL;
	return NULL;
}

static inline void
_rotate_trans_vb(float trans_vb[16], bool clockwise) {
	if (!clockwise) {
		float x, y;
		x = trans_vb[2]; y = trans_vb[3];
		trans_vb[2] = trans_vb[6];  trans_vb[3] = trans_vb[7];
		trans_vb[6] = trans_vb[10]; trans_vb[7] = trans_vb[11];
		trans_vb[10]= trans_vb[14]; trans_vb[11]= trans_vb[15];
		trans_vb[14]= x;            trans_vb[15]= y;	
	} else {
		float x, y;
		x = trans_vb[2]; y = trans_vb[3];
		trans_vb[2] = trans_vb[14];  trans_vb[3] = trans_vb[15];
		trans_vb[14] = trans_vb[10]; trans_vb[15] = trans_vb[11];
		trans_vb[10]= trans_vb[6]; trans_vb[11]= trans_vb[7];
		trans_vb[6]= x;            trans_vb[7]= y;	
	}	
}

static inline void 
_relocate_draw_vb(uint16_t part_src[8], 
                  struct dtex_inv_size* src_sz, 
				  struct dtex_rect* src_rect, 
				  struct dtex_inv_size* dst_sz, 
				  struct dtex_rect* dst_rect, 
				  int rotate, 
				  float trans_vb[16]) {
	float src_xmin = (src_rect->xmin - SRC_EXTRUDE) * src_sz->inv_w,
	      src_xmax = (src_rect->xmax + SRC_EXTRUDE) * src_sz->inv_w,
	      src_ymin = (src_rect->ymin - SRC_EXTRUDE) * src_sz->inv_h,
	      src_ymax = (src_rect->ymax + SRC_EXTRUDE) * src_sz->inv_h;
	float dst_xmin = (dst_rect->xmin - SRC_EXTRUDE) * dst_sz->inv_w,
	      dst_xmax = (dst_rect->xmax + SRC_EXTRUDE) * dst_sz->inv_w,
	      dst_ymin = (dst_rect->ymin - SRC_EXTRUDE) * dst_sz->inv_h,
	      dst_ymax = (dst_rect->ymax + SRC_EXTRUDE) * dst_sz->inv_h;
	float vd_xmin = dst_xmin * 2 - 1,
          vd_xmax = dst_xmax * 2 - 1,
          vd_ymin = dst_ymin * 2 - 1,
          vd_ymax = dst_ymax * 2 - 1;

    if (part_src == NULL || part_src[0] < 0) {
		trans_vb[0] = vd_xmin; 	trans_vb[1] = vd_ymin;
		trans_vb[2] = src_xmin; trans_vb[3] = src_ymin;
		trans_vb[4] = vd_xmin; 	trans_vb[5] = vd_ymax;
		trans_vb[6] = src_xmin; trans_vb[7] = src_ymax;
		trans_vb[8] = vd_xmax; 	trans_vb[9] = vd_ymax;
		trans_vb[10]= src_xmax; trans_vb[11]= src_ymax;
		trans_vb[12]= vd_xmax; 	trans_vb[13]= vd_ymin;
		trans_vb[14]= src_xmax; trans_vb[15]= src_ymin;
    } else {
		float cx = 0, cy = 0;
		for (int i = 0; i < 4; ++i) {
			cx += part_src[i*2];
			cy += part_src[i*2+1];
		}
		cx *= 0.25f;
		cy *= 0.25f;

	    if (part_src[0] < cx) {
			trans_vb[2] = src_xmin; trans_vb[10]= src_xmax;
			trans_vb[0] = vd_xmin; trans_vb[8] = vd_xmax;
	    } else {
			trans_vb[2] = src_xmax; trans_vb[10]= src_xmin;
			trans_vb[0] = vd_xmax; trans_vb[8] = vd_xmin;
	    }
	    if (part_src[2] < cx) {
			trans_vb[6] = src_xmin; trans_vb[14]= src_xmax;
			trans_vb[4] = vd_xmin; trans_vb[12] = vd_xmax;
	    } else {
			trans_vb[6] = src_xmax; trans_vb[14]= src_xmin;
			trans_vb[4] = vd_xmax; trans_vb[12] = vd_xmin;
	    }
	    if (part_src[1] < cy) {
			trans_vb[3] = src_ymin; trans_vb[11]= src_ymax;
			trans_vb[1] = vd_ymin; trans_vb[9] = vd_ymax;
	    } else {
			trans_vb[3] = src_ymax; trans_vb[11]= src_ymin;
			trans_vb[1] = vd_ymax; trans_vb[9] = vd_ymin;
	    }
	    if (part_src[3] < cy) {
			trans_vb[7] = src_ymin; trans_vb[15]= src_ymax;
			trans_vb[5] = vd_ymin; trans_vb[13] = vd_ymax;
	    } else {
			trans_vb[7] = src_ymax; trans_vb[15]= src_ymin;
			trans_vb[5] = vd_ymax; trans_vb[13] = vd_ymin;
	    }
    }

	if (rotate) {
		_rotate_trans_vb(trans_vb, rotate == -1);
	}
}

static inline void
_rotate_dst_vb(float val[8], bool clockwise) {
	if (!clockwise) {
		float x, y;
		x = val[6]; y = val[7];
		val[6] = val[4]; val[7] = val[5];
		val[4] = val[2]; val[5] = val[3];
		val[2] = val[0]; val[3] = val[1];
		val[0] = x;      val[1] = y;
	} else {
		float x, y;
		x = val[6]; y = val[7];
		val[6] = val[0]; val[7] = val[1];
		val[0] = val[2]; val[1] = val[3];
		val[2] = val[4]; val[3] = val[5];
		val[4] = x;      val[5] = y;
	}	
}

static inline void 
_relocate_draw_texcoords(uint16_t part_src[8], 
						 struct dtex_inv_size* dst_sz, 
						 struct dtex_rect* dst_rect, 
						 int rotate, 
						 float val[8]) {
	float dst_xmin = dst_rect->xmin * dst_sz->inv_w,
	      dst_xmax = dst_rect->xmax * dst_sz->inv_w,
	      dst_ymin = dst_rect->ymin * dst_sz->inv_h,
	      dst_ymax = dst_rect->ymax * dst_sz->inv_h;
    if (part_src == NULL || part_src[0] < 0) {
		val[0] = dst_xmin; val[1] = dst_ymax;
		val[4] = dst_xmax; val[5] = dst_ymin;
		val[2] = dst_xmin; val[3] = dst_ymin;
		val[6] = dst_xmax; val[7] = dst_ymax;
    } else {
		float cx = 0, cy = 0;
		for (int i = 0; i < 4; ++i) {
			cx += part_src[i*2];
			cy += part_src[i*2+1];
		}
		cx *= 0.25f;
		cy *= 0.25f;

	    if (part_src[0] < cx) {
			val[0] = dst_xmin; val[4] = dst_xmax;    	
	    } else {
			val[0] = dst_xmax; val[4] = dst_xmin;
	    }
	    if (part_src[2] < cx) {
			val[2] = dst_xmin; val[6] = dst_xmax;
	    } else {
			val[2] = dst_xmax; val[6] = dst_xmin;
	    }
	    if (part_src[1] < cy) {
			val[1] = dst_ymin; val[5] = dst_ymax;
	    } else {
			val[1] = dst_ymax; val[5] = dst_ymin;
	    }
	    if (part_src[3] < cy) {
			val[3] = dst_ymin; val[7] = dst_ymax;
	    } else {
			val[3] = dst_ymax; val[7] = dst_ymin;
	    }
    }
	if (rotate) {
		_rotate_dst_vb(val, rotate == -1);
	}
}

static inline void
_gen_draw_vb(struct dtex_texture* src_tex, struct dtex_texture* dst_tex, 
			 struct dtex_rect* src_rect, struct dtex_rect* dst_rect,
			 bool rotate, float vb[16]) {
	float src_xmin = src_rect->xmin * src_tex->inv_width,
		  src_xmax = src_rect->xmax * src_tex->inv_width,
		  src_ymin = src_rect->ymin * src_tex->inv_height,
		  src_ymax = src_rect->ymax * src_tex->inv_height;
	float dst_xmin = dst_rect->xmin * dst_tex->inv_width * 2 - 1,
		  dst_xmax = dst_rect->xmax * dst_tex->inv_width * 2 - 1,
		  dst_ymin = dst_rect->ymin * dst_tex->inv_height * 2 - 1,
		  dst_ymax = dst_rect->ymax * dst_tex->inv_height * 2 - 1;
	vb[0] = dst_xmin; vb[1] = dst_ymin; vb[2] = src_xmin; vb[3] = src_ymin;
	vb[4] = dst_xmin; vb[5] = dst_ymax; vb[6] = src_xmin; vb[7] = src_ymax;
	vb[8] = dst_xmax; vb[9] = dst_ymax; vb[10]= src_xmax; vb[11]= src_ymax;
	vb[12]= dst_xmax; vb[13]= dst_ymin; vb[14]= src_xmax; vb[15]= src_ymin;
 	if (rotate) {
 		_rotate_trans_vb(vb, false);
 	}
}

static inline void
_set_rect_vb(struct c2_prenode* pn, struct c2_node* n, bool rotate) {
	if (pn->type == C2_PT_SPR) {
		struct dtex_inv_size src_sz;
		src_sz.inv_w = pn->t.SPR.ori_tex->inv_width;
		src_sz.inv_h = pn->t.SPR.ori_tex->inv_height;

		struct dtex_inv_size dst_sz;	
		dst_sz.inv_w = n->dst_tex->inv_width;
		dst_sz.inv_h = n->dst_tex->inv_height;

		int rotate_times = rotate ? 1 : 0;
		_relocate_draw_vb(pn->t.SPR.ej_quad->texture_coord, &src_sz, &pn->t.SPR.rect, &dst_sz, &n->dst_pos->r, rotate_times, n->trans_vb);
		_relocate_draw_texcoords(pn->t.SPR.ej_quad->texture_coord, &dst_sz, &n->dst_pos->r, rotate_times, n->dst_vb);
	} else if (pn->type == C2_PT_TEX) {
		struct dtex_rect* dst_rect = &n->dst_pos->r;
		float dst_inv_w = n->dst_tex->inv_width,
			  dst_inv_h = n->dst_tex->inv_height;
		float dst_xmin = dst_rect->xmin * dst_inv_w,
			  dst_xmax = dst_rect->xmax * dst_inv_w,
			  dst_ymin = dst_rect->ymin * dst_inv_h,
			  dst_ymax = dst_rect->ymax * dst_inv_h;
		float vd_xmin = dst_xmin * 2 - 1,
			  vd_xmax = dst_xmax * 2 - 1,
			  vd_ymin = dst_ymin * 2 - 1,
			  vd_ymax = dst_ymax * 2 - 1;

		n->trans_vb[0] = vd_xmin; n->trans_vb[1] = vd_ymin; n->trans_vb[2] = 0; n->trans_vb[3] = 0;
		n->trans_vb[4] = vd_xmin; n->trans_vb[5] = vd_ymax; n->trans_vb[6] = 0; n->trans_vb[7] = 1;
		n->trans_vb[8] = vd_xmax; n->trans_vb[9] = vd_ymax; n->trans_vb[10]= 1; n->trans_vb[11]= 1;
		n->trans_vb[12]= vd_xmax; n->trans_vb[13]= vd_ymin; n->trans_vb[14]= 1; n->trans_vb[15]= 0;

		n->dst_vb[0] = dst_xmin; n->dst_vb[1] = dst_ymin; 
		n->dst_vb[2] = dst_xmin; n->dst_vb[3] = dst_ymax;
		n->dst_vb[4] = dst_xmax; n->dst_vb[5] = dst_ymax; 
		n->dst_vb[6] = dst_xmax; n->dst_vb[7] = dst_ymin; 
		if (rotate) {
			_rotate_trans_vb(n->trans_vb, false);
			_rotate_dst_vb(n->dst_vb, false);
		}
	} else {
		assert(pn->type == C2_PT_USERDATA);

		// todo copy from C2_PT_TEX, except xmin xmax ymin ymax
		
		struct dtex_rect* dst_rect = &n->dst_pos->r;
		float dst_inv_w = n->dst_tex->inv_width,
			  dst_inv_h = n->dst_tex->inv_height;
		float dst_xmin = dst_rect->xmin * dst_inv_w,
			  dst_xmax = dst_rect->xmax * dst_inv_w,
			  dst_ymin = dst_rect->ymin * dst_inv_h,
			  dst_ymax = dst_rect->ymax * dst_inv_h;
		float vd_xmin = dst_xmin * 2 - 1,
			  vd_xmax = dst_xmax * 2 - 1,
			  vd_ymin = dst_ymin * 2 - 1,
			  vd_ymax = dst_ymax * 2 - 1;
	
		float xmin = (float)pn->t.UD.rect.xmin / pn->t.UD.tex_w,
			  xmax = (float)pn->t.UD.rect.xmax / pn->t.UD.tex_w,
			  ymin = (float)pn->t.UD.rect.ymin / pn->t.UD.tex_h,
			  ymax = (float)pn->t.UD.rect.ymax / pn->t.UD.tex_h;

		n->trans_vb[0] = vd_xmin; n->trans_vb[1] = vd_ymin; n->trans_vb[2] = xmin; n->trans_vb[3] = ymin;
		n->trans_vb[4] = vd_xmin; n->trans_vb[5] = vd_ymax; n->trans_vb[6] = xmin; n->trans_vb[7] = ymax;
		n->trans_vb[8] = vd_xmax; n->trans_vb[9] = vd_ymax; n->trans_vb[10]= xmax; n->trans_vb[11]= ymax;
		n->trans_vb[12]= vd_xmax; n->trans_vb[13]= vd_ymin; n->trans_vb[14]= xmax; n->trans_vb[15]= ymin;

		n->dst_vb[0] = dst_xmin; n->dst_vb[1] = dst_ymin; 
		n->dst_vb[2] = dst_xmin; n->dst_vb[3] = dst_ymax;
		n->dst_vb[4] = dst_xmax; n->dst_vb[5] = dst_ymax; 
		n->dst_vb[6] = dst_xmax; n->dst_vb[7] = dst_ymin; 
		if (rotate) {
			_rotate_trans_vb(n->trans_vb, false);
			_rotate_dst_vb(n->dst_vb, false);
		}
	}
}

struct insert_params {
	struct dtex_c2* c2;
	struct dtex_loader* loader;
	int w;
	int h;
	bool can_clear;
	int static_quad;

	bool rotate;
	struct dtex_tp_pos* pos;
	struct dtex_texture* tex;
	struct tp_index* index;
};

static inline bool
_mode_one_insert_node_quad(struct insert_params* p, float hw, float hh, int quad) {
	struct tp_index* index = &p->c2->t.ONE.index[quad];
	if (index->is_static == p->can_clear) {
		return false;
	}
	p->pos = dtex_tp_add(index->tp, p->w + PADDING*2 + EXTRUDE*2, p->h + PADDING*2 + EXTRUDE*2, true);
	p->rotate = false;
	if (!p->pos) {
		return false;
	}
	p->index = index;
	if (quad == 0 || quad == 1) {
		p->pos->r.ymin += hh;
		p->pos->r.ymax += hh;
	}
	if (quad == 3 || quad == 1) {
		p->pos->r.xmin += hw;
		p->pos->r.xmax += hw;
	}
	return true;
}

static inline bool
_mode_one_insert_node(struct insert_params* p) {
	p->tex = p->c2->t.ONE.texture;
	assert(p->tex->type == DTEX_TT_MID);
	float hw = p->c2->t.ONE.texture->width * 0.5f,
		  hh = p->c2->t.ONE.texture->height * 0.5f;
	if (p->static_quad >= 0) {
		assert(p->static_quad >= 0 && p->static_quad <= 3);
		if (_mode_one_insert_node_quad(p, hw, hh, p->static_quad)) {
			return true;
		}
	} else {
		assert(p->static_quad == -1);
		for (int i = 0; i < 4; ++i) {
			if (_mode_one_insert_node_quad(p, hw, hh, i)) {
				return true;
			}
		}
	}

	if (p->can_clear) {
		// todo
		// 1. use new texture
		// 2. scale
		// 3. clear
		_clear(p->c2, p->loader);
	}
	return false;
}

static inline bool
_mode_multi_insert_node(struct insert_params* p) {
	for (int i = 0; i < p->c2->t.MULTI.tex_size && p->pos == NULL; ++i) {
		p->tex = p->c2->t.MULTI.textures[i];
		assert(p->tex->type == DTEX_TT_MID);
		// todo padding and rotate
		//	if (w >= h) {
		p->pos = dtex_tp_add(p->tex->t.MID.tp, p->w + PADDING*2 + EXTRUDE*2, p->h + PADDING*2 + EXTRUDE*2, true);
		p->rotate = false;
		//	} else {
		//		pos = dtex_tp_add(tex->tp, h, w, true);
		//		rotate = true;
		//	}
	}

	if (p->pos) {
		p->index = &p->c2->t.MULTI.index;
	} else {
		// todo
		// 1. use new texture
		// 2. scale
		// 3. clear
		_clear(p->c2, p->loader);
		return false;
	}
	return true;
}

static inline void
_draw_extrude(struct dtex_texture* src_tex, struct dtex_texture* dst_tex, int src_w, int src_h, struct dtex_rect* dst_rect, bool rotate) {
	float vb[16];
	struct dtex_rect src, dst;

	static const int SRC_EXTRUDE = 1;

	if (!rotate)
	{
		// left
		src.xmin = 0; src.xmax = SRC_EXTRUDE; src.ymin = 0; src.ymax = src_h;
		dst = *dst_rect; dst.xmax = dst.xmin; dst.xmin -= EXTRUDE;
		_gen_draw_vb(src_tex, dst_tex, &src, &dst, rotate, vb);
		dtex_draw_to_texture(src_tex, dst_tex, vb);

		// right
		src.xmin = src_w - SRC_EXTRUDE; src.xmax = src_w; src.ymin = 0; src.ymax = src_h;
		dst = *dst_rect; dst.xmin = dst.xmax; dst.xmax += EXTRUDE;
		_gen_draw_vb(src_tex, dst_tex, &src, &dst, rotate, vb);
		dtex_draw_to_texture(src_tex, dst_tex, vb);

		// top
		src.xmin = 0; src.xmax = src_w; src.ymin = src_h - SRC_EXTRUDE; src.ymax = src_h;
		dst = *dst_rect; dst.ymin = dst.ymax; dst.ymax += EXTRUDE;
		_gen_draw_vb(src_tex, dst_tex, &src, &dst, rotate, vb);
		dtex_draw_to_texture(src_tex, dst_tex, vb);

		// bottom
		src.xmin = 0; src.xmax = src_w; src.ymin = 0; src.ymax = SRC_EXTRUDE;
		dst = *dst_rect; dst.ymax = dst.ymin; dst.ymin -= EXTRUDE;
		_gen_draw_vb(src_tex, dst_tex, &src, &dst, rotate, vb);
		dtex_draw_to_texture(src_tex, dst_tex, vb);

		// left-top
		src.xmin = 0; src.xmax = SRC_EXTRUDE; src.ymin = src_h - SRC_EXTRUDE; src.ymax = src_h;
		dst = *dst_rect; dst.xmax = dst.xmin; dst.xmin -= EXTRUDE; dst.ymin = dst.ymax; dst.ymax += EXTRUDE;
		_gen_draw_vb(src_tex, dst_tex, &src, &dst, rotate, vb);
		dtex_draw_to_texture(src_tex, dst_tex, vb);

		// right-top
		src.xmin = src_w - SRC_EXTRUDE; src.xmax = src_w; src.ymin = src_h - SRC_EXTRUDE; src.ymax = src_h;
		dst = *dst_rect; dst.xmin = dst.xmax; dst.xmax += EXTRUDE; dst.ymin = dst.ymax; dst.ymax += EXTRUDE;
		_gen_draw_vb(src_tex, dst_tex, &src, &dst, rotate, vb);
		dtex_draw_to_texture(src_tex, dst_tex, vb);

		// left-bottom
		src.xmin = 0; src.xmax = SRC_EXTRUDE; src.ymin = 0; src.ymax = SRC_EXTRUDE;
		dst = *dst_rect; dst.xmax = dst.xmin; dst.xmin -= EXTRUDE; dst.ymax = dst.ymin; dst.ymin -= EXTRUDE;
		_gen_draw_vb(src_tex, dst_tex, &src, &dst, rotate, vb);
		dtex_draw_to_texture(src_tex, dst_tex, vb);

		// right-bottom
		src.xmin = src_w - SRC_EXTRUDE; src.xmax = src_w; src.ymin = 0; src.ymax = SRC_EXTRUDE;
		dst = *dst_rect; dst.xmin = dst.xmax; dst.xmax += EXTRUDE; dst.ymax = dst.ymin; dst.ymin -= EXTRUDE;
		_gen_draw_vb(src_tex, dst_tex, &src, &dst, rotate, vb);
		dtex_draw_to_texture(src_tex, dst_tex, vb);
	} 
	else
	{
		// left
		src.xmin = 0; src.xmax = SRC_EXTRUDE; src.ymin = 0; src.ymax = src_h;
		dst = *dst_rect; dst.ymax = dst.ymin; dst.ymin -= EXTRUDE;
		_gen_draw_vb(src_tex, dst_tex, &src, &dst, rotate, vb);
		dtex_draw_to_texture(src_tex, dst_tex, vb);

		// right
		src.xmin = src_w - SRC_EXTRUDE; src.xmax = src_w; src.ymin = 0; src.ymax = src_h;
		dst = *dst_rect; dst.ymin = dst.ymax; dst.ymax += EXTRUDE;
		_gen_draw_vb(src_tex, dst_tex, &src, &dst, rotate, vb);
		dtex_draw_to_texture(src_tex, dst_tex, vb);

		// top
		src.xmin = 0; src.xmax = src_w; src.ymin = src_h - SRC_EXTRUDE; src.ymax = src_h;
		dst = *dst_rect; dst.xmax = dst.xmin; dst.xmin -= EXTRUDE;
		_gen_draw_vb(src_tex, dst_tex, &src, &dst, rotate, vb);
		dtex_draw_to_texture(src_tex, dst_tex, vb);

		// bottom
		src.xmin = 0; src.xmax = src_w; src.ymin = 0; src.ymax = SRC_EXTRUDE;
		dst = *dst_rect; dst.xmin = dst.xmax; dst.xmax += EXTRUDE;
		_gen_draw_vb(src_tex, dst_tex, &src, &dst, rotate, vb);
		dtex_draw_to_texture(src_tex, dst_tex, vb);

 		// left-top
 		src.xmin = 0; src.xmax = SRC_EXTRUDE; src.ymin = src_h - SRC_EXTRUDE; src.ymax = src_h;
		dst = *dst_rect; dst.xmax = dst.xmin; dst.xmin -= EXTRUDE; dst.ymax = dst.ymin; dst.ymin -= EXTRUDE;
 		_gen_draw_vb(src_tex, dst_tex, &src, &dst, rotate, vb);
 		dtex_draw_to_texture(src_tex, dst_tex, vb);
 
 		// right-top
 		src.xmin = src_w - SRC_EXTRUDE; src.xmax = src_w; src.ymin = src_h - SRC_EXTRUDE; src.ymax = src_h;
		dst = *dst_rect; dst.xmax = dst.xmin; dst.xmin -= EXTRUDE; dst.ymin = dst.ymax; dst.ymax += EXTRUDE;
 		_gen_draw_vb(src_tex, dst_tex, &src, &dst, rotate, vb);
 		dtex_draw_to_texture(src_tex, dst_tex, vb);
 
 		// left-bottom
 		src.xmin = 0; src.xmax = SRC_EXTRUDE; src.ymin = 0; src.ymax = SRC_EXTRUDE;
		dst = *dst_rect; dst.xmin = dst.xmax; dst.xmax += EXTRUDE; dst.ymax = dst.ymin; dst.ymin -= EXTRUDE;
 		_gen_draw_vb(src_tex, dst_tex, &src, &dst, rotate, vb);
 		dtex_draw_to_texture(src_tex, dst_tex, vb);
 
 		// right-bottom
 		src.xmin = src_w - SRC_EXTRUDE; src.xmax = src_w; src.ymin = 0; src.ymax = SRC_EXTRUDE;
		dst = *dst_rect; dst.xmin = dst.xmax; dst.xmax += EXTRUDE; dst.ymin = dst.ymax; dst.ymax += EXTRUDE;
 		_gen_draw_vb(src_tex, dst_tex, &src, &dst, rotate, vb);
 		dtex_draw_to_texture(src_tex, dst_tex, vb);
	}
}

static inline struct c2_node*
_insert_node(struct dtex_c2* c2, struct dtex_loader* loader, struct c2_prenode* pn, struct tp_index** tp) {
	struct insert_params ip;
	ip.c2 = c2;
	ip.loader = loader;
	if (pn->type == C2_PT_SPR) {
		struct c2_node* node = _query_node(c2, pn->t.SPR.pkg->id, pn->t.SPR.spr_id, pn->t.SPR.quad_idx, tp);
		if (node) {
			return node;
		}
		ip.w = pn->t.SPR.rect.xmax - pn->t.SPR.rect.xmin;
		ip.h = pn->t.SPR.rect.ymax - pn->t.SPR.rect.ymin;
		ip.can_clear = dtex_c2_insert_can_clear(pn->t.SPR.pkg->c2_stg);
	} else if (pn->type == C2_PT_TEX) {
		ip.w = pn->t.TEX.w;
		ip.h = pn->t.TEX.h;
		ip.can_clear = true;
	} else {
		assert(pn->type == C2_PT_USERDATA);
		ip.w = pn->t.UD.rect.xmax - pn->t.UD.rect.xmin;
		ip.h = pn->t.UD.rect.ymax - pn->t.UD.rect.ymin;
		ip.can_clear = true;
	}

	//// rrp
	//struct rrp_picture* rrp_pic = NULL;
	//if (pn->rect.xmin < 0) {
	//	struct dtex_rrp* rrp = dtexloader_query_rrp(loader, pn->ej_pkg);
	//	if (rrp == NULL) {
	//		return;
	//	}
	//	rrp_pic = dtex_rrp_get_pic(rrp, -pn->rect.xmin);
	//	assert(rrp_pic);
	//	w = rrp_pic->w;
	//	h = rrp_pic->h;
	//}

	ip.static_quad = c2->static_quad;
	ip.rotate = false;
	ip.pos = NULL;
	ip.tex = NULL;
	ip.index = NULL;
	bool inserted;
	if (c2->one_tex_mode) {
		inserted = _mode_one_insert_node(&ip);
	} else {
		inserted = _mode_multi_insert_node(&ip);
	}
	if (!inserted) {
		return NULL;
	}
    
    ip.rotate = (ip.pos->is_rotated && !ip.rotate) ||
		        (!ip.pos->is_rotated && ip.rotate);

	// save info

	struct c2_node* node = NULL;
	if (ip.index->node_count == MAX_NODE_COUNT) {
		LOGW("%s", "c2 nodes empty.");
		return NULL;
	}
	node = &ip.index->nodes[ip.index->node_count++];

	assert(ip.tex);
	node->dst_tex = ip.tex;
	node->dst_pos = ip.pos;
	node->dst_pos->r.xmin += PADDING + EXTRUDE;
	node->dst_pos->r.ymin += PADDING + EXTRUDE;
	node->dst_pos->r.xmax -= PADDING + EXTRUDE;
	node->dst_pos->r.ymax -= PADDING + EXTRUDE;

	struct dtex_texture tex;
	if (pn->type == C2_PT_SPR) {
		node->hk.pkg_id = pn->t.SPR.pkg->id;
		node->hk.spr_id = pn->t.SPR.spr_id;
		node->hk.quad_idx = pn->t.SPR.quad_idx;
		tex = *pn->t.SPR.ori_tex;
	} else if (pn->type == C2_PT_TEX) {
		node->hk.pkg_id = DEFAULT_PKG_ID;
		node->hk.spr_id = pn->t.TEX.key;
		node->hk.quad_idx = DEFAULT_QUAD_IDX;
		tex.id = pn->t.TEX.id;
		tex.width = pn->t.TEX.w;
		tex.height = pn->t.TEX.h;
		tex.inv_width = 1.0f / tex.width;
		tex.inv_height = 1.0f / tex.height;
		tex.uid = 0;
		tex.type = DTEX_TT_RAW;
		tex.t.RAW.id_alpha = 0;
		tex.t.RAW.format = DTEX_PNG8;
		tex.t.RAW.scale = tex.t.RAW.lod_scale = 1;
	} else {
		assert(pn->type == C2_PT_USERDATA);
		node->hk.pkg_id = DEFAULT_PKG_ID;
		node->hk.spr_id = pn->t.UD.userdata;
		node->hk.quad_idx = DEFAULT_QUAD_IDX;
		tex.id = pn->t.TEX.id;
		tex.width = pn->t.TEX.w;
		tex.height = pn->t.TEX.h;
		tex.inv_width = 1.0f / tex.width;
		tex.inv_height = 1.0f / tex.height;
		tex.uid = 0;
		tex.type = DTEX_TT_RAW;
		tex.t.RAW.id_alpha = 0;
		tex.t.RAW.format = DTEX_PNG8;
		tex.t.RAW.scale = tex.t.RAW.lod_scale = 1;
	}

	ds_hash_insert(ip.index->hash, &node->hk, node, true);
	_set_rect_vb(pn, node, ip.rotate);

	ip.pos->ud = node;

// 	if (rrp_pic) {
// 		dtex_draw_rrp_to_tex(&tex, rrp_pic, tex, pos, rotate);
// 	} else {
		dtex_draw_to_texture(&tex, ip.tex, node->trans_vb);
//	}

	if (pn->type == C2_PT_TEX) {
		_draw_extrude(&tex, ip.tex, ip.w, ip.h, &ip.pos->r, ip.rotate);
	}

	*tp = ip.index;

	return node;
}

static inline bool
_is_same_quad(struct c2_prenode* n0, struct c2_prenode* n1) {
	if (n0->type != n1->type) {
		return false;
	}
	if (n0->type == C2_PT_SPR) {
		if (n0->t.SPR.pkg != n1->t.SPR.pkg || 
			n0->t.SPR.ori_tex != n1->t.SPR.ori_tex ||
			n0->t.SPR.ej_quad->texid != n1->t.SPR.ej_quad->texid) {
			return false;
		} else {
			return n0->t.SPR.rect.xmin == n1->t.SPR.rect.xmin
				&& n0->t.SPR.rect.ymin == n1->t.SPR.rect.ymin
				&& n0->t.SPR.rect.xmax == n1->t.SPR.rect.xmax
				&& n0->t.SPR.rect.ymax == n1->t.SPR.rect.ymax;
		}
	} else if (n0->type == C2_PT_TEX) {
		return n0->t.TEX.key == n1->t.TEX.key;
	} else if (n0->type == C2_PT_USERDATA) {
		return n0->t.UD.userdata = n1->t.UD.userdata;
	} else {
		return false;
	}
}

void 
dtex_c2_load_end(struct dtex_c2* c2, struct dtex_loader* loader) {
	if (--c2->loadable > 0 || c2->prenode_size == 0) {
		return;
	}

	struct c2_prenode* unique_set[c2->prenode_size];
	int unique_sz = 0;
	_get_unique_prenodes(c2, unique_set, &unique_sz);
//	LOGD(" c2 end count: %d", unique_sz);

	// insert
	dtex_shader_scissor(false);
	dtex_draw_begin();
	qsort((void*)unique_set, unique_sz, sizeof(struct c2_prenode*), _compare_max_edge);	
	struct c2_prenode* last_src = NULL;
	struct c2_node* last_dst = NULL;
	struct tp_index* last_tp = NULL;
	for (int i = 0; i < unique_sz; ++i) {
		struct c2_prenode* curr_src = unique_set[i];
		struct c2_node* curr_dst = NULL;
		struct tp_index* curr_tp = NULL;
		if (last_src && _is_same_quad(last_src, curr_src)) {
			assert(last_dst && last_tp);
			curr_tp = last_tp;
			if (last_tp->node_count == MAX_NODE_COUNT) {
				LOGW("%s", "c2 nodes empty.");
				break;
			}
			curr_dst = &last_tp->nodes[last_tp->node_count++];
			curr_dst->dst_tex = last_dst->dst_tex;
			curr_dst->dst_pos = last_dst->dst_pos;
			memcpy(curr_dst->dst_vb, last_dst->dst_vb, sizeof(curr_dst->dst_vb));
			memcpy(curr_dst->trans_vb, last_dst->trans_vb, sizeof(curr_dst->trans_vb));
			if (curr_src->type == C2_PT_SPR) {
				curr_dst->hk.pkg_id = curr_src->t.SPR.pkg->id;
				curr_dst->hk.spr_id = curr_src->t.SPR.spr_id;
				curr_dst->hk.quad_idx = curr_src->t.SPR.quad_idx;
			} else if (curr_src->type == C2_PT_TEX) {
				curr_dst->hk.pkg_id = DEFAULT_PKG_ID;
				curr_dst->hk.spr_id = curr_src->t.TEX.key;
				curr_dst->hk.quad_idx = DEFAULT_QUAD_IDX;
			} else {
				assert(curr_src->type == C2_PT_USERDATA);
				curr_dst->hk.pkg_id = DEFAULT_PKG_ID;
				curr_dst->hk.spr_id = curr_src->t.UD.userdata;
				curr_dst->hk.quad_idx = DEFAULT_QUAD_IDX;				
			}
			ds_hash_insert(curr_tp->hash, &curr_dst->hk, curr_dst, true);
		} else {
			curr_dst = _insert_node(c2, loader, curr_src, &curr_tp);
			if (!curr_dst) {
				break;
			}
		}
		last_src = curr_src;
		last_dst = curr_dst;
		last_tp = curr_tp;
	}
	dtex_draw_end();
	dtex_shader_scissor(true);

	c2->prenode_size = 0;
}

void 
dtex_c2_remove_tex(struct dtex_c2* c2, int key) {
	struct hash_key hk;
	hk.pkg_id = DEFAULT_PKG_ID;
	hk.spr_id = key;
	hk.quad_idx = DEFAULT_QUAD_IDX;
	if (c2->one_tex_mode) {
		for (int i = 0; i < 4; ++i) {
			struct c2_node* ret = (struct c2_node*)ds_hash_query(c2->t.ONE.index[i].hash, &hk);
			if (ret) {
				ds_hash_remove(c2->t.ONE.index[i].hash, &hk);				
				return;
			}
		}
	} else {
		ds_hash_remove(c2->t.MULTI.index.hash, &hk);
	}
}

void 
dtex_c2_reload_begin(struct dtex_c2* c2) {
	dtex_texture_reload(c2->t.ONE.texture);
	dtex_draw_begin();
}

void 
dtex_c2_reload_tex(struct dtex_c2* c2, int tex_id, int tex_width, int tex_height, int key) {
	if (tex_id <= 0 || tex_width <= 0 || tex_height <= 0) {
		return;
	}

	struct tp_index* tp = NULL;
	struct c2_node* node = _query_node(c2, DEFAULT_PKG_ID, key, DEFAULT_QUAD_IDX, &tp);
	assert(node);

	struct dtex_texture tex;
	tex.id = tex_id;
	tex.width = tex_width;
	tex.height = tex_height;
	tex.inv_width = 1.0f / tex.width;
	tex.inv_height = 1.0f / tex.height;
	tex.uid = 0;
	tex.type = DTEX_TT_RAW;
	tex.t.RAW.id_alpha = 0;
	tex.t.RAW.format = DTEX_PNG8;
	tex.t.RAW.scale = tex.t.RAW.lod_scale = 1;

	dtex_draw_to_texture(&tex, node->dst_tex, node->trans_vb);
}

void 
dtex_c2_reload_end() {
	dtex_draw_end();
}

float* 
dtex_c2_query_spr(struct dtex_c2* c2, int pkg_id, int spr_id, int quad_idx, int* out_texid) {
	struct tp_index* tp = NULL;
	struct c2_node* node = _query_node(c2, pkg_id, spr_id, quad_idx, &tp);
	if (!node) {
		return NULL;
	}

	*out_texid = node->dst_tex->id;

	return node->dst_vb;
}

float* 
dtex_c2_query_tex(struct dtex_c2* c2, int key, int* out_texid) {
	struct tp_index* tp = NULL;
	struct c2_node* node = _query_node(c2, DEFAULT_PKG_ID, key, DEFAULT_QUAD_IDX, &tp);
	if (!node) {
		return NULL;
	}

	*out_texid = node->dst_tex->id;

	return node->dst_vb;
}

float* 
dtex_c2_query_userdata(struct dtex_c2* c2, uint32_t userdata, int* out_texid) {
	struct tp_index* tp = NULL;
	struct c2_node* node = _query_node(c2, DEFAULT_PKG_ID, userdata, DEFAULT_QUAD_IDX, &tp);
	if (!node) {
		return NULL;
	}

	*out_texid = node->dst_tex->id;

	return node->dst_vb;	
}

void 
dtexc2_query_map_addr(struct dtex_c2* c2, int pkg_id, int spr_id, int quad_idx,
                      struct dtex_texture** out_tex, struct dtex_tp_pos** out_pos) {
	struct tp_index* tp = NULL;
	struct c2_node* node = _query_node(c2, pkg_id, spr_id, quad_idx, &tp);
	if (node) {
		*out_tex = node->dst_tex;
		*out_pos = node->dst_pos;
	} else {
		*out_tex = NULL;
		*out_pos = NULL;
	}
}

struct dtex_cg* 
dtex_c2_get_cg(struct dtex_c2* c2) {
	if (c2 && c2->one_tex_mode) {
		return c2->t.ONE.cg;
	} else {
		return NULL;
	}
}

void 
dtex_c2_debug_draw(struct dtex_c2* c2) {
	if (c2->one_tex_mode) {
		dtex_debug_draw(c2->t.ONE.texture->id, 4);
	} else {
		if (c2->t.MULTI.tex_size > 0) {
			dtex_debug_draw(c2->t.MULTI.textures[0]->id, 4);
			if (c2->t.MULTI.tex_size > 1) {
				dtex_debug_draw(c2->t.MULTI.textures[1]->id, 3);
			}
		}
	}
}