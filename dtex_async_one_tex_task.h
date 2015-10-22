#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_async_one_tex_task_h
#define dynamic_texture_async_one_tex_task_h

#include "ejoy2d.h"

struct dtex_package;

void dtex_async_load_one_texture(struct dtex_package* pkg, 
								 int idx, 
								 const char* desc);

#endif // dynamic_texture_async_one_tex_task_h

#ifdef __cplusplus
}
#endif