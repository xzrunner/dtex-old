#ifndef dynamic_texture_async_h
#define dynamic_texture_async_h

struct dtex_loader;
struct dtex_rect;

struct ej_package;

void dtex_async_load_spr(struct dtex_loader*, struct ej_package*, struct dtex_rect*, int spr_id, const char* path);

#endif // dynamic_texture_async_h