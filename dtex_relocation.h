#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_relocation_h
#define dynamic_texture_relocation_h

struct dtex_package;
struct ds_array;

/************************************************************************/
/* swap quad's texid and texcoords with extend info of pkg.             */
/************************************************************************/
void dtex_swap_quad_src_info(struct dtex_package* pkg, struct ds_array* picture_ids);

#endif // dynamic_texture_relocation_h

#ifdef __cplusplus
}
#endif