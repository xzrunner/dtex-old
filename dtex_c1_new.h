#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_cache1_h
#define dynamic_texture_cache1_h

struct dtex_c1;

struct dtex_c1* dtex_c1_create(struct dtex_buffer*);
void dtex_c1_release(struct dtex_c1*, struct dtex_buffer*);

void dtex_c1_update(struct dtex_c1*, struct dtex_package*, int spr_id);

void dtex_c1_debug_draw(struct dtex_c1*);

#endif // dynamic_texture_cache1_h

#ifdef __cplusplus
}
#endif