#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_async_queue_h
#define dynamic_texture_async_queue_h

#include <pthread.h>
#include <stdbool.h>

struct dtex_async_job {
	struct dtex_async_job* next;
	pthread_t id;

	int type;
	void* ud;
};

struct dtex_async_queue;

struct dtex_async_queue* dtex_job_queue_create();
void dtex_job_queue_release(struct dtex_async_queue* qp);

struct dtex_async_job* dtex_job_queue_front_and_pop(struct dtex_async_queue* queue);
void dtex_job_queue_push(struct dtex_async_queue* queue, struct dtex_async_job* job);
//struct dtex_async_job* dtex_job_queue_find(struct dtex_async_queue* queue, pthread_t id);

#endif // dynamic_texture_async_queue_h

#ifdef __cplusplus
}
#endif