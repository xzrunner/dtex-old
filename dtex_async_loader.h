#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_async_loader_h
#define dynamic_texture_async_loader_h

#include <stdbool.h>

struct dtex_import_stream;

void dtex_async_loader_create();
void dtex_async_loader_release();

void dtex_async_loader_clear();

void dtex_async_load_file(const char* filepath, void (*cb)(struct dtex_import_stream* is, void* ud), void* ud, const char* desc);

void dtex_async_loader_update();

bool dtex_async_loader_empty();

#endif // dynamic_texture_async_loader_h

#ifdef __cplusplus
}
#endif