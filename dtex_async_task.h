#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_async_task_h
#define dynamic_texture_async_task_h

#include <ejoy2d.h>

void dtex_async_load_texture(struct dtex_buffer* buf, struct dtex_package* pkg, int idx);

void dtex_async_load_multi_textures(struct dtex_buffer* buf, struct dtex_package* pkg, int* texture_ids, int texture_count, void (*cb)(void* ud), void* ud);

#endif // dynamic_texture_async_task_h

#ifdef __cplusplus
}
#endif