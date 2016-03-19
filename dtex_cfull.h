#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_cache_full_h
#define dynamic_texture_cache_full_h

#include "dtex_typedef.h"

#include "ejoy2d.h"

struct dtex_package;
struct dtex_texture;
struct ds_hash;
struct dtex_tp;

#define DTEX_CF_MAX_NODE_COUNT	128

struct dtex_cf_texture;
struct dtex_cf_node {
	struct dtex_package* pkg;
	// src
	int src_tex_idx;
	// dst
	struct dtex_cf_texture* dst_tex;
	struct dtex_rect dst_rect;
	bool dst_rotated;

	bool finish;	// relocated

	void* ud;
};

struct dtex_cf_prenode {
	struct dtex_package* pkg;
	int tex_idx;
	float scale;
};

struct dtex_cf_texture {
	struct dtex_texture* tex;
	struct dtex_rect region;

	struct dtex_cf_node nodes[DTEX_CF_MAX_NODE_COUNT];
	int node_count;

	struct ds_hash* hash;
	struct dtex_tp* tp;

	void* ud;
};

extern inline int
dtex_cf_prenode_size_cmp(const void* arg1, const void* arg2);

extern inline int
dtex_cf_node_pkg_cmp(const void* arg1, const void* arg2);

void dtex_cf_unique_prenodes(struct dtex_cf_prenode* src_list, int src_sz,
							 struct dtex_cf_prenode** dst_list, int* dst_sz);

bool dtex_cf_pack_prenodes(struct dtex_cf_prenode*, struct dtex_cf_texture*, float scale);

extern inline void
dtex_cf_clear_tex_info(struct dtex_cf_texture*);

extern inline void
dtex_cf_relocate_pic(int pic_id, struct ej_pack_picture* ej_pic, void* ud);

// todo other format: rrr, b4r
extern inline void
dtex_cf_relocate_node(struct dtex_texture* src, struct dtex_cf_node* dst);

#endif // dynamic_texture_cache_full_h

#ifdef __cplusplus
}
#endif