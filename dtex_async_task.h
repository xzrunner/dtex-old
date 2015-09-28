#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_async_task_h
#define dynamic_texture_async_task_h

void dtex_async_load_texture(struct dtex_buffer* buf, const char* filepath, struct dtex_package* pkg, int idx, float scale);

#endif // dynamic_texture_async_task_h

#ifdef __cplusplus
}
#endif