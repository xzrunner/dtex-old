#ifndef dynamic_texture_cache3_h
#define dynamic_texture_cache3_h

struct dtex_c3;
struct dtex_package;
struct dtex_loader;
struct dtex_buffer;

struct dtex_c3* dtexc3_create();
void dtexc3_release(struct dtex_c3*, struct dtex_buffer*);

void dtexc3_preload(struct dtex_c3*, struct dtex_package*, float scale);
void dtexc3_preload_end(struct dtex_c3*, struct dtex_loader*, struct dtex_buffer*);

void dtexc3_relocate(struct dtex_c3*, struct dtex_package*);

struct dtex_package* dtexc3_query_package(struct dtex_c3*, const char* name);
struct dtex_rect* dtexc3_query_rect(struct dtex_c3*, const char* name);

void dtexc3_debug_draw(struct dtex_c3*);

#endif // dynamic_texture_cache3_h