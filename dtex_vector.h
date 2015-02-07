#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_vector_h
#define dynamic_texture_vector_h

struct dtex_vector;

struct dtex_vector* dtex_vector_create(int cap);
void dtex_vector_release(struct dtex_vector*);

int dtex_vector_size(struct dtex_vector*);
void dtex_vector_push_back(struct dtex_vector*, void* item);
void dtex_vector_clear(struct dtex_vector*);

void* dtex_vector_get(struct dtex_vector*, int idx);

#endif // dynamic_texture_vector_h

#ifdef __cplusplus
}
#endif