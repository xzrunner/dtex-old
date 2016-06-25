#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_texture_loader_h
#define dynamic_texture_texture_loader_h

#include <stdint.h>
#include <stdbool.h>

struct dtex_import_stream;
struct dtex_package;
struct dtex_texture;

int dtex_load_all_textures_desc(struct dtex_import_stream* is, struct dtex_package* pkg, float scale);

void dtex_load_texture_only_desc(struct dtex_import_stream* is, struct dtex_texture* tex, float scale);
void dtex_load_texture_all(struct dtex_import_stream* is, struct dtex_texture* tex);

extern inline int 
dtex_load_pvr_tex(const uint8_t* data, int width, int height, int format);

extern inline int 
dtex_load_etc2_tex(const uint8_t* data, int width, int height);

#endif // dynamic_texture_texture_loader_h

#ifdef __cplusplus
}
#endif