#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_package_h
#define dynamic_texture_package_h

#include <ejoy2d.h>

struct dtex_rrp;
struct dtex_pts;
struct dtex_rrr;
struct dtex_b4r;

struct dtex_raw_tex;

#define DTEX_PACK_TEX_SZ 128

struct export_name {
	const char* name;
	int id;
};

struct dtex_package {
	char* name;

	struct dtex_raw_tex* textures[DTEX_PACK_TEX_SZ];		// todo malloc
	char* tex_filepaths[DTEX_PACK_TEX_SZ];
	int tex_size;
	float tex_scale;

	struct export_name* export_names;
	int export_size;

	struct ej_sprite_pack* ej_pkg;	// epe
	struct dtex_rrp* rrp_pkg;		// regular rect pack
	struct dtex_pts* pts_pkg;		// picture triangles strip
	struct dtex_rrr* rrr_pkg;		// regular rect raw (only for pvr)
	struct dtex_b4r* b4r_pkg;		// block4 raw (only for pvr)
};

struct dtex_package* dtex_package_create();
void dtex_package_release(struct dtex_package* pkg);

int dtex_get_spr_id(struct dtex_package* pkg, const char* name);

//void dtex_package_release_textures(struct dtex_package* pkg);

#endif // dynamic_texture_package_h

#ifdef __cplusplus
}
#endif