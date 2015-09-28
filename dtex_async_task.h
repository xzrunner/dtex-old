#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_async_task_h
#define dynamic_texture_async_task_h

void dtex_async_load_texture(struct dtex_buffer* buf, struct dtex_package* pkg, int idx);

void dtex_async_load_texture_with_c2(struct dtex_buffer* buf, struct dtex_package* pkg, int* sprite_ids, int sprite_count);

#endif // dynamic_texture_async_task_h

#ifdef __cplusplus
}
#endif