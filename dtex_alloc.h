#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_alloc_h
#define dynamic_texture_alloc_h

struct alloc;

struct alloc* dtex_init_alloc(int sz);
void* dtex_alloc(struct alloc* a, int sz);

int dtex_alloc_size(struct alloc* a);

#endif // dynamic_texture_alloc_h

#ifdef __cplusplus
}
#endif