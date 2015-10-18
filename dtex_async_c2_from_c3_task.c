#include "dtex_async_c2_from_c3_task.h"
#include "dtex_relocation.h"
#include "dtex_array.h"
#include "dtex_package.h"
#include "dtex_c2.h"
#include "dtex_c3.h"
#include "dtex_texture.h"
#include "dtex_utility.h"
#include "dtex_async_multi_tex_task.h"
#include "dtex_async_queue.h"

#include <pthread.h>

#include <string.h>
#include <stdlib.h>
#include <assert.h>

struct load_params {
	struct load_params* next;

	struct dtex_loader* loader;
	struct dtex_c2* c2;
	struct dtex_c3* c3;

	struct dtex_package* pkg;

	struct dtex_array* spr_ids;
	struct dtex_array* pic_ids;
	struct dtex_array* tex_ids;
};

struct load_params_queue {
	struct load_params* head;
	struct load_params* tail;
	pthread_rwlock_t lock;
};

static struct load_params_queue PARAMS_QUEUE;

void 
dtex_async_load_c2_from_c3_init() {
	DTEX_ASYNC_QUEUE_INIT(PARAMS_QUEUE);
}

static inline void
_cb_func(void* ud) {
	struct load_params* params = (struct load_params*)ud;

	if (!params->c2) {
		dtex_swap_quad_src_info(params->pkg, params->pic_ids);
		DTEX_ASYNC_QUEUE_PUSH(PARAMS_QUEUE, params);
		return;
	}

	struct dtex_package* pkg = params->pkg;

	assert(pkg->c2_loading == 1);
	pkg->c2_loading = 0;

	dtex_swap_quad_src_info(pkg, params->pic_ids);

	dtex_c2_load_begin(params->c2); 
	int spr_sz = dtex_array_size(params->spr_ids);
	for (int i = 0; i < spr_sz; ++i) {
		int spr_id = *(int*)dtex_array_fetch(params->spr_ids, i);
		dtex_c2_load(params->c2, pkg, spr_id);
	}
	dtex_c2_load_end(params->c2, params->loader);

	struct dtex_texture* dst_textures[pkg->texture_count];
	struct dtex_rect* dst_regions[pkg->texture_count];
	dtex_c3_query_map_info(params->c3, pkg, dst_textures, dst_regions);

	int sz = dtex_array_size(params->tex_ids);
	for (int i = 0; i < sz; ++i) {
		int idx = *(int*)dtex_array_fetch(params->tex_ids, i);

		struct dtex_texture *src_tex = pkg->textures[idx], 
			*dst_tex = dst_textures[idx];

		assert(src_tex->type == DTEX_TT_RAW && dst_tex->type == DTEX_TT_MID);
		struct dtex_texture_with_rect src, dst;

		src.tex = src_tex;
		src.rect.xmin = src.rect.ymin = 0;
		src.rect.xmax = src_tex->width;
		src.rect.ymax = src_tex->height;

		dst.tex = dst_tex;
		dst.rect = *dst_regions[idx];

		dtex_relocate_c2_key(params->c2, pkg, idx, params->pic_ids, &src, &dst);
	}

	dtex_swap_quad_src_info(pkg, params->pic_ids);

	for (int i = 0; i < sz; ++i) {
		int idx = *(int*)dtex_array_fetch(params->tex_ids, i);
		dtex_texture_release(pkg->textures[idx]);
	}
	dtex_package_remove_all_textures_ref(pkg);

	DTEX_ASYNC_QUEUE_PUSH(PARAMS_QUEUE, params);
}

bool 
dtex_async_load_c2_from_c3(struct dtex_loader* loader,
                           struct dtex_c2* c2,
                           struct dtex_c3* c3,
                           struct dtex_package* pkg, 
						   int* sprite_ids, 
						   int sprite_count) {
	if (pkg->c2_loading) {
		return false;
	}
	pkg->c2_loading = 1;

	struct load_params* params = NULL;
	DTEX_ASYNC_QUEUE_POP(PARAMS_QUEUE, params);
	if (!params) {
		params = (struct load_params*)malloc(sizeof(*params));
		params->spr_ids = dtex_array_create(100, sizeof(int));
		params->pic_ids = dtex_array_create(10, sizeof(int));
		params->tex_ids = dtex_array_create(10, sizeof(int));
	}

	dtex_array_clear(params->spr_ids);
	for (int i = 0; i < sprite_count; ++i) {
		dtex_array_add(params->spr_ids, &sprite_ids[i]);
	}

	// swap to origin data, get texture idx info
	dtex_get_picture_id_unique_set(pkg->ej_pkg, sprite_ids, sprite_count, params->pic_ids);
	dtex_swap_quad_src_info(pkg, params->pic_ids);
	dtex_get_texture_id_unique_set(pkg->ej_pkg, sprite_ids, sprite_count, params->tex_ids);
	dtex_swap_quad_src_info(pkg, params->pic_ids);

	params->loader = loader;
	params->c2 = c2;
	params->c3 = c3;
	params->pkg = pkg;
	
	dtex_package_change_lod(pkg, 0);
	dtex_async_load_multi_textures(pkg, params->tex_ids, _cb_func, params, "c2 from c3");

	return true;
}