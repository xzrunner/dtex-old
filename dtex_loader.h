#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_loader_h
#define dynamic_texture_loader_h

#include <stdbool.h>

struct dtex_loader;
struct dtex_import_stream;
struct dtex_package;
struct dtex_texture;

struct dtex_loader* dtexloader_create();
void dtexloader_release(struct dtex_loader*);

struct dtex_package* dtex_load_pkg(struct dtex_loader*, const char* name, const char* epe_path, float scale, int lod);
void dtex_unload_pkg(struct dtex_loader*, struct dtex_package* pkg);

int dtex_preload_all_textures(const char* filepath, struct dtex_loader*, struct dtex_package* pkg, float scale);
void dtex_preload_texture(struct dtex_loader*, struct dtex_package* pkg, int idx, float scale);
void dtex_load_texture(struct dtex_loader*, struct dtex_package*, int idx);
void dtex_load_texture_raw(struct dtex_loader*, const char* path, struct dtex_texture*);

void dtex_load_file(const char* filepath, void (*unpack_func)(struct dtex_import_stream* is, void* ud), void* ud);

void dtex_package_traverse(struct dtex_loader*, void (*pkg_func)(struct dtex_package* pkg));
struct dtex_package* dtex_query_pkg(struct dtex_loader*, const char* name);

//// old loader's 
//struct dtex_raw_tex* dtexloader_load_image(const char* filepath);
//struct dtex_rrp* dtexloader_query_rrp(struct dtex_loader*, struct ej_sprite_pack*);
//struct dtex_pts* dtexloader_query_pts(struct dtex_loader*, struct ej_sprite_pack*);

#endif // dynamic_texture_loader_h

#ifdef __cplusplus
}
#endif
