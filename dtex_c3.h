#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_cache3_h
#define dynamic_texture_cache3_h

struct dtex_c3;
struct dtex_package;
struct dtex_loader;
struct dtex_buffer;
struct dtex_texture;
struct dtex_raw_tex;

struct dtex_c3* dtexc3_create();
void dtexc3_release(struct dtex_c3*, struct dtex_buffer*);

void dtexc3_preload_pkg(struct dtex_c3*, struct dtex_package*, float scale);
void dtexc3_preload_pkg_end(struct dtex_c3*, struct dtex_loader*, struct dtex_buffer*);

// todo cache and sort
struct dp_position* dtexc3_load_tex(struct dtex_c3*, struct dtex_raw_tex*, struct dtex_buffer*, struct dtex_texture** dst);

void dtexc3_relocate(struct dtex_c3*, struct dtex_package*);

struct dtex_package* dtexc3_query_pkg(struct dtex_c3*, const char* name);
// todo: tex_idx
struct dtex_rect* dtexc3_query_rect(struct dtex_c3*, const char* name);

void dtexc3_debug_draw(struct dtex_c3*);

#endif // dynamic_texture_cache3_h

#ifdef __cplusplus
}
#endif