#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_loader_h
#define dynamic_texture_loader_h

#include "opengl.h"

#define DTEX_PACK_TEX_SZ 16

struct dtex_rrp;
struct dtex_pts;

struct dtex_raw_tex {
	GLuint id;
	GLuint id_alpha;	// for etc1
	int width, height;
	int format;
};

struct dtex_package {
	char* name;
	char* filepath;	

	struct dtex_raw_tex textures[DTEX_PACK_TEX_SZ];
	int tex_size;
	float tex_scale;

	struct ej_package* ej_pkg;		// epd
	struct dtex_rrp* rrp_pkg;		// regular rect pack
	struct dtex_pts* pts_pkg;		// picture triangles strip
	struct dtex_rrr* rrr_pkg;		// regular rect raw (only for pvr)
	struct dtex_b4r* b4r_pkg;		// block4 raw (only for pvr)
};

struct dtex_rect;

struct dtex_loader;

struct dtex_loader* dtexloader_create();
void dtexloader_release(struct dtex_loader*);

struct dtex_package* dtexloader_preload_pkg(struct dtex_loader*, const char* name, const char* path);
struct dtex_raw_tex* dtexloader_load_epp(struct dtex_loader*, struct dtex_package*, int idx);
struct dtex_raw_tex* dtexloader_load_image(const char* path);
void dtexloader_unload_tex(struct dtex_raw_tex*);

struct dtex_package* dtexloader_get_pkg(struct dtex_loader*, int idx);

void dtexloader_load_spr2task(struct dtex_loader*, struct ej_package*, struct dtex_rect**, int id, const char* path);
void dtexloader_do_task(struct dtex_loader*, void (*on_load_func)());
void dtexloader_after_do_task(struct dtex_loader*, void (*after_load_func)());

struct dtex_rrp* dtexloader_query_rrp(struct dtex_loader*, struct ej_package*);
struct dtex_pts* dtexloader_query_pts(struct dtex_loader*, struct ej_package*);

#endif // dynamic_texture_loader_h

#ifdef __cplusplus
}
#endif