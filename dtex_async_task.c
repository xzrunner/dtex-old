#include "dtex_async_task.h"
#include "dtex_package.h"
#include "dtex_texture_loader.h"
#include "dtex_async_loader.h"
#include "dtex_texture_pool.h"

#include "dtex_facade.h"

#include <assert.h>
#include <stdlib.h>

//////////////////////////////////////////////////////////////////////////
// only load texture
//////////////////////////////////////////////////////////////////////////

struct load_texture_params {
	struct dtex_buffer* buf;
	struct dtex_raw_tex* tex;
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
	dtex_async_load_file(pkg->textures[idx]->filepath, _load_texture_func, params);
}

//////////////////////////////////////////////////////////////////////////
// load texture and load sprites to C2
//////////////////////////////////////////////////////////////////////////

struct load_texture_and_c2_share_params {
	struct dtex_buffer* buf;

	struct dtex_package* pkg;

	int tot_count;
	int loaded_count;

	int* sprite_ids;
	int sprite_count;	
};

struct load_texture_and_c2_params {
	struct load_texture_and_c2_share_params* share_params;
	int tex_idx;
};

static inline void
_load_texture_with_c2_func(struct dtex_import_stream* is, void* ud) {
	struct load_texture_and_c2_params* params = (struct load_texture_and_c2_params*)ud;	
	struct load_texture_and_c2_share_params* share_params = params->share_params;

	dtex_load_texture_all(share_params->buf, is, share_params->pkg->textures[params->tex_idx]);

	free(params);

	++share_params->loaded_count;
	if (share_params->loaded_count != share_params->tot_count) {
		return;
	}

	dtexf_c2_load_begin();
	for (int i = 0; i < share_params->sprite_count; ++i) {
		dtexf_c2_load(share_params->pkg, share_params->sprite_ids[i]);
	}
	dtexf_c2_load_end();

	free(share_params);
}

void 
dtex_async_load_texture_with_c2(struct dtex_buffer* buf, struct dtex_package* pkg, int* sprite_ids, int sprite_count) {
	struct load_texture_and_c2_share_params* share_params = (struct load_texture_and_c2_share_params*)malloc(sizeof(*share_params));

	share_params->buf = buf;

	share_params->pkg = pkg;

	share_params->tot_count = pkg->tex_size;
	share_params->loaded_count = 0;

	share_params->sprite_ids = sprite_ids;
	share_params->sprite_count = sprite_count;

	for (int i = 0; i < pkg->tex_size; ++i) {
		struct load_texture_and_c2_params* params = (struct load_texture_and_c2_params*)malloc(sizeof(*params));
		params->share_params = share_params;
		params->tex_idx = i;
		dtex_async_load_file(pkg->textures[i]->filepath, _load_texture_with_c2_func, params);
	}
}