#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_package_h
#define dynamic_texture_package_h

#include "ejoy2d.h"

struct dtex_rrp;
struct dtex_pts;
struct dtex_rrr;
struct dtex_b4r;

#define DTEX_PACK_TEX_SZ 128

struct export_name {
	const char* name;
	int id;
};

struct quad_ext_info {
	int texid;
	uint16_t texture_coord[8];
};

struct pic_ext_info {
	struct quad_ext_info quads[1];
};

struct dtex_package {
	char* name;
	char* filepath;

	struct dtex_texture* textures[DTEX_PACK_TEX_SZ];
	int texture_count;
	int LOD;

	struct export_name* export_names;
	int export_size;

	void** spr_ext_info;

	struct ej_sprite_pack* ej_pkg;	// epe
	struct dtex_rrp* rrp_pkg;		// regular rect pack
	struct dtex_pts* pts_pkg;		// picture triangles strip
	struct dtex_rrr* rrr_pkg;		// regular rect raw (only for pvr)
	struct dtex_b4r* b4r_pkg;		// block4 raw (only for pvr)
};

struct dtex_package* dtex_package_create();
void dtex_package_release(struct dtex_package*);

int dtex_get_spr_id(struct dtex_package*, const char* name);

//void dtex_package_release_textures(struct dtex_package*);

void dtex_package_remove_texture_ref(struct dtex_package*, struct dtex_texture*);
void dtex_package_remove_all_textures_ref(struct dtex_package*);

int dtex_package_texture_idx(struct dtex_package*, struct dtex_texture*);

void dtex_package_change_lod(struct dtex_package*, int lod);

#endif // dynamic_texture_package_h

#ifdef __cplusplus
}
#endif