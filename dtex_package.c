#include "dtex_package.h"
#include "dtex_lod.h"
#include "dtex_texture.h"
#include "dtex_c2_strategy.h"
#include "dtex_c3_strategy.h"
#include "dtex_res_path.h"

#include <string.h>
#include <stdlib.h>

//struct dtex_package* 
//dtex_package_create() {
//	struct dtex_package* pkg = (struct dtex_package*)malloc(sizeof(*pkg));
//	memset(pkg, 0, sizeof(*pkg));
//// todo id
//	return pkg;
//}

void 
dtex_package_release(struct dtex_package* pkg) {
 	free(pkg->name);

	dtex_res_path_release(pkg->rp);

 	for (int i = 0; i < pkg->texture_count; ++i) {
 		dtex_texture_release(pkg->textures[i]);
 	}

	for (int i = 0, n = pkg->export_size; i < n; ++i) {
		struct export_name* ep = &pkg->export_names[i];
		free((char*)(ep->name));
	}
	free(pkg->export_names);

	free(pkg->spr_ext_info);

 	free(pkg->ej_pkg);
 	// 	dtex_rrp_release(pkg->rrp_pkg);
 	// 	dtex_pts_release(pkg->pts_pkg);
 	// 	dtex_rrr_release(pkg->rrr_pkg);
 	// 	dtex_b4r_release(pkg->b4r_pkg);
 
 	dtex_c3_strategy_release(pkg->c3_stg);
 	dtex_c2_strategy_release(pkg->c2_stg);

//	free(pkg);

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
	for (int i = 0; i < pkg->texture_count; ++i) {
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
	for (int i = 0; i < pkg->texture_count; ++i) {
		if (pkg->textures[i] == tex) {
			return i;
		}
	}
	return -1;
}

void 
dtex_package_change_lod(struct dtex_package* pkg, int lod) {
	if (pkg->LOD == lod) {
		return;
	}

	pkg->LOD = lod;
	float scale = dtex_lod_get_scale(lod);
	for (int i = 0; i < pkg->texture_count; ++i) {
		if (!pkg->textures[i]) {
			continue;
		}
		pkg->textures[i]->t.RAW.lod_scale = scale;
	}
}