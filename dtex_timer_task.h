#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_timer_task_h
#define dynamic_texture_timer_task_h

void dtex_timer_task_init();

void dtex_timer_task_update();

void dtex_timer_task_init_add(int time, void (*cb)(void* ud), void (*cb_clear)(void* ud), void* ud);

#endif // dynamic_texture_timer_task_h

#ifdef __cplusplus
}
#endif