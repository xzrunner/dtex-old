#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_loader_h
#define dynamic_texture_loader_h

#include <stdbool.h>

struct dtex_loader;
struct dtex_import_stream;

struct dtex_loader* dtexloader_create();
void dtexloader_release(struct dtex_loader*);

struct dtex_package* dtex_load_pkg(struct dtex_loader*, const char* name, const char* filepath, int format, float scale, int lod, int load_c2);
void dtex_preload_texture(struct dtex_loader*, struct dtex_package* pkg, int idx, float scale);
void dtex_load_texture(struct dtex_loader*, struct dtex_package*, int idx, bool create_by_ej);

void dtex_load_file(const char* filepath, void (*unpack_func)(struct dtex_import_stream* is, void* ud), void* ud);

struct dtex_package* dtex_fetch_pkg(struct dtex_loader*, int idx);
struct dtex_package* dtex_query_pkg(struct dtex_loader*, const char* name);

//// old loader's 
//struct dtex_raw_tex* dtexloader_load_image(const char* filepath);
//struct dtex_rrp* dtexloader_query_rrp(struct dtex_loader*, struct ej_sprite_pack*);
//struct dtex_pts* dtexloader_query_pts(struct dtex_loader*, struct ej_sprite_pack*);

#endif // dynamic_texture_loader_h

#ifdef __cplusplus
}
#endif
