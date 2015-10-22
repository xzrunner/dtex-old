#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_array_h
#define dynamic_texture_array_h

#include <stddef.h>

struct dtex_array;

struct dtex_array* dtex_array_create(int cap, size_t data_sz);
void dtex_array_release(struct dtex_array*);

int dtex_array_size(struct dtex_array*);
void* dtex_array_fetch(struct dtex_array*, int idx);

void dtex_array_add(struct dtex_array*, void* data);
void dtex_array_clear(struct dtex_array*);

#endif // dynamic_texture_array_h

#ifdef __cplusplus
}
#endif