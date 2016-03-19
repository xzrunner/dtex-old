#include "dtex_res_cache.h"
#include "dtex_texture.h"
#include "dtex_target.h"

#include <ds_stack.h>

#include <stdlib.h>
#include <string.h>

#define MAX_TEXTURE_SIZE 4096
#define MID_TEXTURE_SIZE 2048
#define MIN_TEXTURE_SIZE 1024

struct cache {
	struct ds_stack* max_tex_list;
	struct ds_stack* mid_tex_list;
	struct ds_stack* min_tex_list;

	struct ds_stack* target_list;
};

static struct cache C;

void
dtex_res_cache_create() {
	C.max_tex_list = ds_stack_create(2, sizeof(struct dtex_texture*));
	C.mid_tex_list = ds_stack_create(4, sizeof(struct dtex_texture*));
	C.min_tex_list = ds_stack_create(6, sizeof(struct dtex_texture*));

	C.target_list = ds_stack_create(2, sizeof(struct dtex_target*));
}

static inline void
_release_texture_list(struct ds_stack* tex_list) {
	while (!ds_stack_empty(tex_list)) {
		struct dtex_texture* texture = *(struct dtex_texture**)ds_stack_top(tex_list);
		ds_stack_pop(tex_list);
		dtex_texture_release(texture);
	}
	ds_stack_release(tex_list);
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
	_release_texture_list(C.max_tex_list); C.max_tex_list = NULL;
	_release_texture_list(C.mid_tex_list); C.mid_tex_list = NULL;
	_release_texture_list(C.min_tex_list); C.min_tex_list = NULL;

	_release_target_list(C.target_list); C.target_list = NULL;
}

static struct dtex_texture*
_texture_list_fetch(struct ds_stack* tex_list) {
	if (!ds_stack_empty(tex_list)) {
		struct dtex_texture* texture = *(struct dtex_texture**)ds_stack_top(tex_list);
		ds_stack_pop(tex_list);
		return texture;
	} else {
		return NULL;
	}
}

struct dtex_texture* 
dtex_res_cache_fetch_mid_texture(int edge) {
	struct dtex_texture* tex = NULL;
	if (edge == MAX_TEXTURE_SIZE) {
		tex = _texture_list_fetch(C.max_tex_list);
	} else if (edge == MID_TEXTURE_SIZE) {
		tex = _texture_list_fetch(C.mid_tex_list);
	} else if (edge == MIN_TEXTURE_SIZE) {
		tex = _texture_list_fetch(C.min_tex_list);
	}

	if (!tex) {
		tex = dtex_texture_create_mid(edge);
	}

	return tex;
}

static inline void
_list_return(struct ds_stack* tex_list, struct dtex_texture* tex) {
	ds_stack_push(tex_list, &tex);
}

bool
dtex_res_cache_return_mid_texture(struct dtex_texture* tex) {
	if (tex->width != tex->height || tex->type != DTEX_TT_MID) {
		return false;
	}

	bool succ = true;
	if (tex->width == MAX_TEXTURE_SIZE) {
		_list_return(C.max_tex_list, tex);
	} else if (tex->width == MID_TEXTURE_SIZE) {
		_list_return(C.mid_tex_list, tex);
	} else if (tex->width == MIN_TEXTURE_SIZE) {
		_list_return(C.min_tex_list, tex);
	} else {
		succ = false;
	}
	return succ;
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