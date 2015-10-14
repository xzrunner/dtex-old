#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_async_task_h
#define dynamic_texture_async_task_h

#include "ejoy2d.h"

void dtex_async_load_texture(struct dtex_package* pkg, int idx, const char* desc);

void dtex_async_load_multi_textures(struct dtex_package* pkg, struct dtex_array* texture_idx, void (*cb)(void* ud), void* ud, const char* desc);

void dtex_async_load_epe(const char* filepath, void (*cb)(void* ud), void* ud);

#endif // dynamic_texture_async_task_h

#ifdef __cplusplus
}
#endif