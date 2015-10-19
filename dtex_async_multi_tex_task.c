#include "dtex_async_multi_tex_task.h"
#include "dtex_async_queue.h"
#include "dtex_package.h"
#include "dtex_texture_loader.h"
#include "dtex_async_loader.h"
#include "dtex_texture.h"
#include "dtex_array.h"
#include "dtex_resource.h"
#include "dtex_log.h"

#include <pthread.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct share_params {
	struct share_params* next;

	int tot_count;
	int loaded_count;

	void (*cb)(void* ud);
	void* ud;
};

struct share_params_queue {
	struct share_params* head;
	struct share_params* tail;
	pthread_rwlock_t lock;
};

static struct share_params_queue PARAMS_SHEAR_QUEUE;

struct params {
	struct params* next;

	struct share_params* share_params;
	struct dtex_texture* tex;
};

struct params_queue {
	struct params* head;
	struct params* tail;
	pthread_rwlock_t lock;
};

static struct params_queue PARAMS_QUEUE;

void 
dtex_async_load_multi_textures_init() {
	DTEX_ASYNC_QUEUE_INIT(PARAMS_SHEAR_QUEUE);
	DTEX_ASYNC_QUEUE_INIT(PARAMS_QUEUE);
}

static inline void
_load_multi_textures_func(struct dtex_import_stream* is, void* ud) {
	struct params* params = (struct params*)ud;	
	struct share_params* share_params = params->share_params;

	dtex_load_texture_all(is, params->tex, true);

	DTEX_ASYNC_QUEUE_PUSH(PARAMS_QUEUE, params);

	++share_params->loaded_count;
	if (share_params->loaded_count != share_params->tot_count) {
		return;
	}

	if (share_params->cb) {
		share_params->cb(share_params->ud);
	}

	DTEX_ASYNC_QUEUE_PUSH(PARAMS_SHEAR_QUEUE, share_params);
}

void 
dtex_async_load_multi_textures(struct dtex_package* pkg, struct dtex_array* texture_idx, 
							   void (*cb)(void* ud), void* ud, const char* desc) {
	struct share_params* share_params = NULL;
	DTEX_ASYNC_QUEUE_POP(PARAMS_SHEAR_QUEUE, share_params);
	if (!share_params) {
		share_params = (struct share_params*)malloc(sizeof(*share_params));
	}

	int sz = dtex_array_size(texture_idx);
	share_params->tot_count = sz;
	share_params->loaded_count = 0;

	share_params->cb = cb;
	share_params->ud = ud;

	for (int i = 0; i < sz; ++i) {
		int idx = *(int*)dtex_array_fetch(texture_idx, i);
		assert(idx < pkg->texture_count);

		struct params* params = NULL;
		DTEX_ASYNC_QUEUE_POP(PARAMS_QUEUE, params);
		if (!params) {
			params = (struct params*)malloc(sizeof(*params));
		}
		params->share_params = share_params;

		if (pkg->textures[idx] == NULL) {
			pkg->textures[idx] = dtex_texture_create_raw(pkg->LOD);
			pkg->textures[idx]->t.RAW.scale = 1;
		} else {
			dtex_info(" already exist, small scale.");
			assert(0);
		}
		params->tex = pkg->textures[idx];

		char path_full[strlen(pkg->filepath) + 10];
		dtex_get_texture_filepath(pkg->filepath, idx, pkg->LOD, path_full);
		dtex_async_load_file(path_full, _load_multi_textures_func, params, desc);
	}
}