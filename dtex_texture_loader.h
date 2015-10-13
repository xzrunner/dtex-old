#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_texture_loader_h
#define dynamic_texture_texture_loader_h

#include <stdint.h>
#include <stdbool.h>

struct dtex_import_stream;

void dtex_load_texture_only_desc(struct dtex_import_stream* is, struct dtex_texture* tex, float scale);
void dtex_load_texture_all(struct dtex_import_stream* is, struct dtex_texture* tex, bool create_by_ej);

#endif // dynamic_texture_texture_loader_h

#ifdef __cplusplus
}
#endif