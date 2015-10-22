#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_texture_loader_h
#define dynamic_texture_texture_loader_h

#include <stdbool.h>

void dtex_texture_cache_init(int cap);

bool dtex_texture_cache_insert(struct dtex_texture*, struct dtex_package*, int idx);
struct dtex_texture* dtex_texture_cache_query(struct dtex_package*, int idx);

#endif // dynamic_texture_texture_loader_h

#ifdef __cplusplus
}
#endif