#include "dtex_async_one_tex_task.h"
#include "dtex_package.h"
#include "dtex_texture_loader.h"
#include "dtex_async_loader.h"
#include "dtex_resource.h"

#include <string.h>

static inline void
_load_texture_func(struct dtex_import_stream* is, void* ud) {
	struct dtex_texture* tex = (struct dtex_texture*)ud;	
 	dtex_load_texture_all(is, tex, true);
}

void 
dtex_async_load_one_texture(struct dtex_package* pkg, int idx, const char* desc) {
	char path_full[strlen(pkg->filepath) + 10];
	dtex_get_texture_filepath(pkg->filepath, idx, pkg->LOD, path_full);
	dtex_async_load_file(path_full, _load_texture_func, pkg->textures[idx], desc);
}