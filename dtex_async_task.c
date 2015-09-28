#include "dtex_async_task.h"
#include "dtex_package.h"
#include "dtex_texture_loader.h"
#include "dtex_async_loader.h"

#include <assert.h>
#include <stdlib.h>

// only load texture

struct load_texture_params {
	struct dtex_buffer* buf;
	struct dtex_package* pkg;
	int idx;
	float scale;
};

static inline void
_load_texture_func(struct dtex_import_stream* is, void* ud) {
	struct load_texture_params* params = (struct load_texture_params*)ud;
	
 	struct dtex_raw_tex* tex = params->pkg->textures[params->idx];
 	assert(tex);
 	dtex_load_texture_all(params->buf, is, tex);
}

void 
dtex_async_load_texture(struct dtex_buffer* buf, const char* filepath, struct dtex_package* pkg, int idx, float scale) {
	struct load_texture_params* params = (struct load_texture_params*)malloc(sizeof(*params));
	params->buf = buf;
	params->pkg = pkg;
	params->idx = idx;
	params->scale = scale;

	dtex_async_load_file(filepath, _load_texture_func, params);
}