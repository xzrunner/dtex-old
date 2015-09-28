#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_target_h
#define dynamic_texture_target_h

struct dtex_target;
struct dtex_texture;

struct dtex_target* dtex_new_target();
void dtex_del_target(struct dtex_target*);

void dtex_target_bind_texture(struct dtex_target*, int texid);
void dtex_target_unbind_texture(struct dtex_target*);

void dtex_target_bind(struct dtex_target*);
void dtex_target_unbind();

#endif // dynamic_texture_target_h

#ifdef __cplusplus
}
#endif