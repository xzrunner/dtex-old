#include "dtex_package.h"

#include <string.h>
#include <stdlib.h>

struct dtex_package* 
dtex_package_create() {
	struct dtex_package* pkg = (struct dtex_package*)malloc(sizeof(*pkg));
	memset(pkg, 0, sizeof(*pkg));
	return pkg;
}

void 
dtex_package_release(struct dtex_package* pkg) {
	for (int i = 0; i < pkg->tex_size; ++i) {
		free(pkg->tex_filepaths[i]);
	}

	free(pkg->ej_pkg);
	// 	dtex_rrp_release(pkg->rrp_pkg);
	// 	dtex_pts_release(pkg->pts_pkg);
	// 	dtex_rrr_release(pkg->rrr_pkg);
	// 	dtex_b4r_release(pkg->b4r_pkg);

	free(pkg->name);

	memset(pkg, 0, sizeof(*pkg));
}

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

// void 
// dtex_package_release_textures(struct dtex_package* pkg) {
// 	for (int i = 0; i < pkg->tex_size; ++i) {
// 		dtex_pool_remove(pkg->textures[i]);
// 	}
// }

void 
dtex_package_remove_texture_ref(struct dtex_package* pkg, struct dtex_texture* tex) {
	for (int i = 0; i < pkg->tex_size; ++i) {
		if (pkg->textures[i] == tex) {
			pkg->textures[i] = NULL;
		}
	}	
}

void 
dtex_package_remove_all_textures_ref(struct dtex_package* pkg) {
	memset(pkg->textures, 0, sizeof(pkg->textures));
}

int 
dtex_package_texture_idx(struct dtex_package* pkg, struct dtex_texture* tex) {
	int idx = -1;
	for (int i = 0; i < pkg->tex_size; ++i) {
		if (pkg->textures[i] == tex) {
			idx = i;
			break;
		}
	}
	return idx;
}