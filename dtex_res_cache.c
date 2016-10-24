#include "dtex_res_cache.h"
#include "dtex_texture.h"
#include "dtex_target.h"

#include <ds_stack.h>
#include <ds_array.h>

#include <stdlib.h>
#include <string.h>

struct cache {
	struct ds_array* texture_list;
	struct ds_stack* target_list;
};

static struct cache C;

#define MAX_MID_TEX_COUNT 64

void
dtex_res_cache_create() {
	C.texture_list = ds_array_create(MAX_MID_TEX_COUNT, sizeof(struct dtex_texture*));	
	C.target_list = ds_stack_create(2, sizeof(struct dtex_target*));
}

static inline void
_release_texture_list(struct ds_array* tex_list) {
	for (int i = 0, n = ds_array_size(tex_list); i < n; ++i) {
		struct dtex_texture* texture = *(struct dtex_texture**)ds_array_fetch(tex_list, i);
		dtex_texture_release(texture);
	}
	ds_array_release(tex_list);
}

static inline void
_release_target_list(struct ds_stack* tar_list) {
	while (!ds_stack_empty(tar_list)) {
		struct dtex_target* target = *(struct dtex_target**)ds_stack_top(tar_list);
		ds_stack_pop(tar_list);
		dtex_target_release(target);
	}
	ds_stack_release(tar_list);
}

void 
dtex_res_cache_release() {
	_release_texture_list(C.texture_list); C.texture_list = NULL;
	_release_target_list(C.target_list); C.target_list = NULL;
}

static struct dtex_texture*
_texture_list_fetch(struct ds_array* tex_list, int width, int height) {
	for (int i = 0, n = ds_array_size(tex_list); i < n; ++i) {
		struct dtex_texture* texture = *(struct dtex_texture**)ds_array_fetch(tex_list, i);
		if (texture->width == width && texture->height == height) {
			return texture;
		}
	}
	return NULL;
}

struct dtex_texture* 
dtex_res_cache_fetch_mid_texture(int width, int height) {
	struct dtex_texture* tex = _texture_list_fetch(C.texture_list, width, height);
	if (!tex) {
		tex = dtex_texture_create_mid(width, height);
	}
	return tex;
}

void
dtex_res_cache_return_mid_texture(struct dtex_texture* tex) {
	if (tex->type == DTEX_TT_MID) {
		ds_array_add(C.texture_list, tex);
	}
}

struct dtex_target* 
dtex_res_cache_fetch_target() {
	if (!ds_stack_empty(C.target_list)) {
		struct dtex_target* target = *(struct dtex_target**)ds_stack_top(C.target_list);
		ds_stack_pop(C.target_list);
		return target;
	} else {
		return dtex_target_create();
	}
}

void 
dtex_res_cache_return_target(struct dtex_target* target) {
	ds_stack_push(C.target_list, &target);
}