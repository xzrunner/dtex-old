#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_resource_path_h
#define dynamic_texture_resource_path_h

struct dtex_res_path;

struct dtex_res_path* dtex_res_path_create(int img_count, int lod_count);
void dtex_res_path_release(struct dtex_res_path*);

void dtex_set_epe_filepath(struct dtex_res_path*, const char* path);
const char* dtex_get_epe_filepath(struct dtex_res_path*);

void dtex_set_ept_filepath(struct dtex_res_path*, const char* path);
const char* dtex_get_ept_filepath(struct dtex_res_path*);

void dtex_set_img_filepath(struct dtex_res_path*, const char* path, int idx, int lod);
const char* dtex_get_img_filepath(struct dtex_res_path*, int idx, int lod);

#endif // dynamic_texture_resource_path_h

#ifdef __cplusplus
}
#endif