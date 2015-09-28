#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_async_loader_h
#define dynamic_texture_async_loader_h

void dtex_async_loader_init();
void dtex_async_loader_release();

void dtex_async_load_file(const char* filepath, int filetype, struct dtex_package*, int idx, float scale);

void dtex_async_loader_update(struct dtex_buffer*);

#endif // dynamic_texture_async_loader_h

#ifdef __cplusplus
}
#endif