#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_texture_pool_h
#define dynamic_texture_texture_pool_h

struct dtex_raw_tex;

void dtedx_pool_init();

int dtex_pool_add(struct dtex_raw_tex* tex);

struct dtex_raw_tex* dtex_pool_query(int id);

int dtex_pool_query_glid(int id);

#endif // dynamic_texture_texture_pool_h

#ifdef __cplusplus
}
#endif