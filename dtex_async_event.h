#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_async_event_h
#define dynamic_texture_async_event_h

#include <pthread.h>
#include <stdbool.h>

struct dtex_event {
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	bool triggered;
};

void 
dtex_event_init(struct dtex_event* ev) {
	pthread_mutex_init(&ev->mutex, 0);
	pthread_cond_init(&ev->cond, 0);
	ev->triggered = false;
}

void 
dtex_event_trigger(struct dtex_event* ev) {
	pthread_mutex_lock(&ev->mutex);
	ev->triggered = true;
	pthread_cond_signal(&ev->cond);
	pthread_mutex_unlock(&ev->mutex);
}

void 
dtex_event_reset(struct dtex_event* ev) {
	pthread_mutex_lock(&ev->mutex);
	ev->triggered = false;
	pthread_mutex_unlock(&ev->mutex);
}

void 
dtex_event_wait(struct dtex_event* ev) {
	pthread_mutex_lock(&ev->mutex);
	while (!ev->triggered)
		pthread_cond_wait(&ev->cond, &ev->mutex);
	pthread_mutex_unlock(&ev->mutex);
}

#endif // dynamic_texture_async_event_h

#ifdef __cplusplus
}
#endif