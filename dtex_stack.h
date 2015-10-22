#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_stack_h
#define dynamic_texture_stack_h

#include <stdbool.h>
#include <stddef.h>
	
struct dtex_stack;

struct dtex_stack* dtex_stack_create(int cap, size_t data_sz);
void dtex_stack_release(struct dtex_stack*);

bool dtex_stack_empty(struct dtex_stack*);
void dtex_stack_push(struct dtex_stack*, void* data);
void* dtex_stack_top(struct dtex_stack*);
void dtex_stack_pop(struct dtex_stack*);

#endif // dynamic_texture_stack_h

#ifdef __cplusplus
}
#endif