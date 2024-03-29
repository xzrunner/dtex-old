#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_target_h
#define dynamic_texture_target_h

struct dtex_target;

void dtex_target_stack_init();
void dtex_target_stack_release();

struct dtex_target* dtex_target_create();
void dtex_target_release(struct dtex_target*);

void dtex_target_bind_texture(struct dtex_target*, int texid);
void dtex_target_unbind_texture(struct dtex_target*);

void dtex_target_bind(struct dtex_target*);
void dtex_target_unbind();

#endif // dynamic_texture_target_h

#ifdef __cplusplus
}
#endif