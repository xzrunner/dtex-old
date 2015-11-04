#include "dtex_async_one_tex_task.h"
#include "dtex_package.h"
#include "dtex_texture_loader.h"
#include "dtex_async_loader.h"
#include "dtex_res_path.h"

#include <string.h>

static inline void
_load_texture_func(struct dtex_import_stream* is, void* ud) {
	struct dtex_texture* tex = (struct dtex_texture*)ud;	
 	dtex_load_texture_all(is, tex, true);
}

void 
dtex_async_load_one_texture(struct dtex_package* pkg, int idx, const char* desc) {
	const char* path = dtex_get_img_filepath(pkg->rp, idx, pkg->LOD);
	dtex_async_load_file(path, _load_texture_func, pkg->textures[idx], desc);
}