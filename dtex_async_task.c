#include "dtex_async_task.h"
#include "dtex_package.h"
#include "dtex_texture_loader.h"
#include "dtex_async_loader.h"
#include "dtex_desc_loader.h"
#include "dtex_texture.h"
#include "dtex_array.h"

#include <assert.h>
#include <stdlib.h>

/************************************************************************/
/* load one texture                                                     */
/************************************************************************/

struct load_texture_params {
	struct dtex_buffer* buf;
	struct dtex_texture* tex;
};

static inline void
_load_texture_func(struct dtex_import_stream* is, void* ud) {
	struct load_texture_params* params = (struct load_texture_params*)ud;	
 	dtex_load_texture_all(params->buf, is, params->tex);
	free(params);
}

void 
dtex_async_load_texture(struct dtex_buffer* buf, struct dtex_package* pkg, int idx) {
	struct load_texture_params* params = (struct load_texture_params*)malloc(sizeof(*params));
	params->buf = buf;
	params->tex = pkg->textures[idx];
	dtex_async_load_file(pkg->texture_filepaths[idx], _load_texture_func, params);
}

/************************************************************************/
/* load multi textures                                                  */
/************************************************************************/

struct load_multi_textures_share_params {
	struct dtex_buffer* buf;

	int tot_count;
	int loaded_count;

	void (*cb)(void* ud);
	void* ud;
};

struct load_multi_textures_params {
	struct load_multi_textures_share_params* share_params;
	struct dtex_texture* tex;
};

static inline void
_load_multi_textures_func(struct dtex_import_stream* is, void* ud) {
	struct load_multi_textures_params* params = (struct load_multi_textures_params*)ud;	
	struct load_multi_textures_share_params* share_params = params->share_params;

	dtex_load_texture_all(share_params->buf, is, params->tex);

	free(params);

	++share_params->loaded_count;
	if (share_params->loaded_count != share_params->tot_count) {
		return;
	}

	if (share_params->cb) {
		share_params->cb(share_params->ud);
	}

	free(share_params);
}

void 
dtex_async_load_multi_textures(struct dtex_buffer* buf, struct dtex_package* pkg, 
							   struct dtex_array* texture_idx, void (*cb)(void* ud), void* ud) {
	struct load_multi_textures_share_params* share_params = (struct load_multi_textures_share_params*)malloc(sizeof(*share_params));

	share_params->buf = buf;

	int sz = dtex_array_size(texture_idx);
	share_params->tot_count = sz;
	share_params->loaded_count = 0;

	share_params->cb = cb;
	share_params->ud = ud;

	for (int i = 0; i < sz; ++i) {
		int idx = *(int*)dtex_array_fetch(texture_idx, i);
		assert(idx < pkg->texture_count);

		struct load_multi_textures_params* params = (struct load_multi_textures_params*)malloc(sizeof(*params));
		params->share_params = share_params;

		if (pkg->textures[idx] == NULL) {
			pkg->textures[idx] = dtex_texture_create_raw();
			pkg->textures[idx]->t.RAW.scale = 1;
		}
		params->tex = pkg->textures[idx];

		dtex_async_load_file(pkg->texture_filepaths[idx], _load_multi_textures_func, params);
	}
}

/************************************************************************/
/* load epe                                                             */
/************************************************************************/

struct load_epe_params {
	void (*cb)(void* ud);
	void* ud;
};

static inline void
_load_epe_func(struct dtex_import_stream* is, void* ud) {
	struct load_epe_params* params = (struct load_epe_params*)ud;	

	struct dtex_package* pkg = dtex_package_create();
	dtex_load_epe(is, pkg, 1);
	

	free(params);
}

void 
dtex_async_load_epe(const char* filepath, void (*cb)(void* ud), void* ud) {
	struct load_epe_params* params = (struct load_epe_params*)malloc(sizeof(*params));
	params->cb = cb;
	params->ud = ud;
	dtex_async_load_file(filepath, _load_epe_func, params);
}