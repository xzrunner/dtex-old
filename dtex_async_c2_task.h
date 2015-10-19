#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_async_c2_task_h
#define dynamic_texture_async_c2_task_h

#include <stdbool.h>

void dtex_async_load_c2_init();

bool dtex_async_load_c2(struct dtex_loader*,
                        struct dtex_c2*,
						struct dtex_package*, 
						int* sprite_ids, 
						int sprite_count);

#endif // dynamic_texture_async_c2_task_h

#ifdef __cplusplus
}
#endif