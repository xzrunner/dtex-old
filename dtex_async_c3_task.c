#include "dtex_async_c3_task.h"
#include "dtex_async_queue.h"
#include "dtex_relocation.h"
#include "dtex_package.h"
#include "dtex_array.h"
#include "dtex_utility.h"
#include "dtex_async_multi_tex_task.h"

#include <pthread.h>

#include <assert.h>
#include <stdlib.h>

struct load_params {
	struct load_params* next;

	struct dtex_loader* loader;
	struct dtex_c2* c2;

	struct dtex_package* pkg;

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
dtex_async_load_c3_init() {
	DTEX_ASYNC_QUEUE_INIT(PARAMS_QUEUE);
}

static inline void
_cb_func(void* ud) {
	struct load_params* params = (struct load_params*)ud;

	dtex_swap_quad_src_info(params->pkg, params->pic_ids);

	DTEX_ASYNC_QUEUE_PUSH(PARAMS_QUEUE, params);
}

bool dtex_async_load_c3(struct dtex_loader* loader,
                        struct dtex_c3* c3,
						struct dtex_package* pkg, 
						int* sprite_ids, 
						int sprite_count) {
	if (pkg->c3_loading) {
		return false;
	}

	struct load_params* params = NULL;
	DTEX_ASYNC_QUEUE_POP(PARAMS_QUEUE, params);
	if (!params) {
		params = (struct load_params*)malloc(sizeof(*params));
		params->pic_ids = dtex_array_create(10, sizeof(int));
		params->tex_ids = dtex_array_create(10, sizeof(int));
	}

	// swap to origin data, get texture idx info
	dtex_get_picture_id_unique_set(pkg->ej_pkg, sprite_ids, sprite_count, params->pic_ids);
	dtex_swap_quad_src_info(pkg, params->pic_ids);
	dtex_get_texture_id_unique_set(pkg->ej_pkg, sprite_ids, sprite_count, params->tex_ids);
	dtex_swap_quad_src_info(pkg, params->pic_ids);

	params->pkg = pkg;

	int tex_sz = dtex_array_size(params->tex_ids);
	int tex_ids[tex_sz];
	for (int i = 0; i < tex_sz; ++i) {
		tex_ids[i] = *(int*)dtex_array_fetch(params->tex_ids, i);
	}

	if (tex_sz == 0) {
		_cb_func(params);
	} else {
		dtex_async_load_multi_textures(pkg, tex_ids, tex_sz, _cb_func, params, "c3");
	}

	return true;
}
