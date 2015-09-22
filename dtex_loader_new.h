#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_loader_h
#define dynamic_texture_loader_h

#include "opengl.h"

#include "ejoy2d.h"

#include <stdbool.h>

#define DTEX_PACK_TEX_SZ 128

struct dtex_rrp;
struct dtex_pts;
struct dtex_rrr;
struct dtex_b4r;

struct dtex_raw_tex;

// todo remove
#define TEXTURE4 0
#define TEXTURE8 1
//#define DETAIL 2
#define PVRTC  3
//#define KTX 4
//#define PKM 5
#define PKMC 6
//#define RRP 11
//#define PTS 12
//#define RRR 13
//#define B4R 14

#define FILE_EPT 0
#define FILE_EPE 1
#define FILE_RRP 2
#define FILE_PTS 3
#define FILE_RRR 4
#define FILE_B4R 5

struct dtex_package {
	char* name;

	struct dtex_raw_tex* textures[DTEX_PACK_TEX_SZ];		// todo malloc
	int tex_size;
	float tex_scale;

	struct ej_sprite_pack* ej_pkg;	// epe
	struct dtex_rrp* rrp_pkg;		// regular rect pack
	struct dtex_pts* pts_pkg;		// picture triangles strip
	struct dtex_rrr* rrr_pkg;		// regular rect raw (only for pvr)
	struct dtex_b4r* b4r_pkg;		// block4 raw (only for pvr)
};

struct dtex_loader;

struct dtex_loader* dtexloader_create();
void dtexloader_release(struct dtex_loader*);

struct dtex_package* dtex_preload_pkg(struct dtex_loader*, const char* name, const char* path, int type);
void dtex_load_texture(struct dtex_loader*, struct dtex_package*, int idx);

struct dtex_package* dtex_get_pkg(struct dtex_loader*, int idx);

#endif // dynamic_texture_loader_h

#ifdef __cplusplus
}
#endif