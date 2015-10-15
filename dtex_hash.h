#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_hash_h
#define dynamic_texture_hash_h

#include <stdbool.h>

struct dtex_hash;
struct dtex_array;

struct dtex_hash* dtex_hash_create(int capacity, int hash_size, float rehash_weight,
	                               unsigned int (*hash_func)(int hash_sz, void* key), 
								   bool (*equal_func)(void* key0, void* key1));
void dtex_hash_release(struct dtex_hash*);

void* dtex_hash_query(struct dtex_hash*, void* key);
void dtex_hash_query_all(struct dtex_hash*, void* key, struct dtex_array* ret);

void dtex_hash_insert(struct dtex_hash*, void* key, void* val, bool force);
void* dtex_hash_remove(struct dtex_hash*, void* key);
void dtex_hash_clear(struct dtex_hash*);

unsigned int dtex_string_hash_func(int hash_sz, void* key);
bool dtex_string_equal_func(void* key0, void* key1);

#endif // dynamic_texture_hash_h

#ifdef __cplusplus
}
#endif