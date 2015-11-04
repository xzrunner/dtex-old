#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_lod_h
#define dynamic_texture_lod_h

void dtex_lod_init(int lod[3]);
float dtex_lod_get_scale(int lod);

#endif // dynamic_texture_lod_h

#ifdef __cplusplus
}
#endif