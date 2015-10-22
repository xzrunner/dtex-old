#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_resources_cache_h
#define dynamic_resources_cache_h

#include <stdbool.h>

struct dtex_texture;
struct dtex_target;

void dtex_res_cache_create();
void dtex_res_cache_release();

struct dtex_texture* dtex_res_cache_fetch_mid_texture(int edge);
bool dtex_res_cache_return_mid_texture(struct dtex_texture*);

struct dtex_target* dtex_res_cache_fetch_target();
void dtex_res_cache_return_target(struct dtex_target*);

#endif // dynamic_resources_cache_h

#ifdef __cplusplus
}
#endif