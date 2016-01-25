#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_async_queue_h
#define dynamic_texture_async_queue_h

#define DTEX_ASYNC_QUEUE_INIT(queue) do { \
	(queue).head = NULL; \
	(queue).tail = NULL; \
	pthread_rwlock_init(&(queue).lock, NULL); \
} while (0)

#define DTEX_ASYNC_QUEUE_POP(queue, front) do { \
	pthread_rwlock_wrlock(&(queue).lock); \
	(front) = (queue).head; \
	if ((queue).head != NULL) { \
		(queue).head = (queue).head->next; \
	} \
	pthread_rwlock_unlock(&(queue).lock); \
} while (0)

#define DTEX_ASYNC_QUEUE_PUSH(queue, back) do { \
	pthread_rwlock_wrlock(&(queue).lock); \
	(back)->next = NULL; \
	if ((queue).head == NULL) { \
		(queue).head = (back); \
		(queue).tail = (back); \
	} else { \
		assert((queue).tail != NULL); \
		(queue).tail->next = (back); \
		(queue).tail = (back); \
	} \
	pthread_rwlock_unlock(&(queue).lock); \
} while (0)

#define DTEX_ASYNC_QUEUE_EMPTY(queue) \
	(pthread_rwlock_rdlock(&queue.lock), queue.head == NULL)

#define DTEX_ASYNC_QUEUE_CLEAR(queue, type) do { \
	pthread_rwlock_wrlock(&(queue).lock); \
 	type* curr = (queue).head; \
	while (curr) { \
		type* next = curr->next; \
		free(curr); \
		curr = next; \
	} \
 	(queue).head = NULL; \
 	(queue).tail = NULL; \
	pthread_rwlock_unlock(&(queue).lock); \
} while (0)

#define DTEX_ASYNC_QUEUE_CLEAR2(queue, type, release_cb) do { \
	pthread_rwlock_wrlock(&(queue).lock); \
	type* curr = (queue).head; \
	while (curr) { \
		type* next = curr->next; \
		if (release_cb) { \
			release_cb(curr); \
		} \
		free(curr); \
		curr = next; \
	} \
	(queue).head = NULL; \
	(queue).tail = NULL; \
	pthread_rwlock_unlock(&(queue).lock); \
} while (0)

//#define DTEX_ASYNC_QUEUE_FOREACH(queue, e) for(e=(queue)->head.next; (e)!=&(queue)->head; (e)=(e)->next)

#endif // dynamic_texture_async_queue_h

#ifdef __cplusplus
}
#endif