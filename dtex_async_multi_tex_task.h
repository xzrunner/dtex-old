#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_async_multi_tex_task_h
#define dynamic_texture_async_multi_tex_task_h

#include "ejoy2d.h"

void dtex_async_load_multi_textures_init();

void dtex_async_load_multi_textures(struct dtex_package* pkg, 
									int* texture_idx_list,
									int texture_idx_sz,
									void (*cb)(void* ud), 
									void* ud, 
									const char* desc);

#endif // dynamic_texture_async_multi_tex_task_h

#ifdef __cplusplus
}
#endif