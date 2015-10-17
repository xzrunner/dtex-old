#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_async_epe_task_h
#define dynamic_texture_async_epe_task_h

#include "ejoy2d.h"

void dtex_async_load_epe(const char* filepath, void (*cb)(void* ud), void* ud);

#endif // dynamic_texture_async_epe_task_h

#ifdef __cplusplus
}
#endif