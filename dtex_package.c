#include "dtex_package.h"
#include "dtex_texture_pool.h"

#include <string.h>

int 
dtex_get_spr_id(struct dtex_package* pkg, const char* name) {
	int start = 0;
	int end = pkg->export_size - 1;
	while (start <= end) {
		int mid = (start + end) / 2;
		int r = strcmp(name, pkg->export_names[mid].name);
		if (r == 0) {
			return pkg->export_names[mid].id;
		} else if (r < 0) {
			end = mid - 1;
		} else {
			start = mid + 1;
		}
	}
	return -1;
}

void 
dtex_package_release_textures(struct dtex_package* pkg) {
	for (int i = 0; i < pkg->tex_size; ++i) {
		dtex_pool_remove(pkg->textures[i]);
	}
}