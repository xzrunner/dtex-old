#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_loader_h
#define dynamic_texture_loader_h

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