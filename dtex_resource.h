#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_resource_h
#define dynamic_texture_resource_h

void dtex_get_resource_filepath(const char* filepath, int format, char* ret);
void dtex_get_texture_filepath(const char* filepath, int idx, int lod, char* ret);

#endif // dynamic_texture_resource_h

#ifdef __cplusplus
}
#endif