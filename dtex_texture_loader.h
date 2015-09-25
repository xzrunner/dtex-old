#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_texture_loader_h
#define dynamic_texture_texture_loader_h

#include <stdint.h>

struct dtex_import_stream;

void dtex_load_texture_desc(struct dtex_import_stream* is, struct dtex_raw_tex* tex, float scale);
void dtex_load_texture_all(struct dtex_buffer*, struct dtex_import_stream* is, struct dtex_raw_tex* tex, float scale);

#endif // dynamic_texture_texture_loader_h

#ifdef __cplusplus
}
#endif