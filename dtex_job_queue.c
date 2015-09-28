#include "dtex_job_queue.h"

#include <stdlib.h>
#include <assert.h>

struct dtex_job_queue {
	struct dtex_job* head;
	struct dtex_job* tail;
	pthread_rwlock_t lock;
};

static inline int
_job_queue_init(struct dtex_job_queue* queue)
{
	int err;

	queue->head = NULL;
	queue->tail = NULL;
	err = pthread_rwlock_init(&queue->lock, NULL);
	if (err != 0)
		return(err);

	return 0;
}

struct dtex_job_queue* 
dtex_job_queue_create() {
	struct dtex_job_queue* qp = (struct dtex_job_queue*)malloc(sizeof(*qp));
	_job_queue_init(qp);
	return qp;
}

void 
dtex_job_queue_release(struct dtex_job_queue* qp) {
	free(qp);
}

struct dtex_job* 
dtex_job_queue_front_and_pop(struct dtex_job_queue* queue) {
	struct dtex_job* front = NULL;	
	pthread_rwlock_wrlock(&queue->lock);
	front = queue->head;
	if (queue->head != NULL) {
		queue->head = queue->head->next;
	}
	pthread_rwlock_unlock(&queue->lock);
	return front;
}

void 
dtex_job_queue_push(struct dtex_job_queue* queue, struct dtex_job* job) {
	pthread_rwlock_wrlock(&queue->lock);
	job->next = NULL;
	if (queue->head == NULL) {
		queue->head = job;
		queue->tail = job;
	} else {
		assert(queue->tail != NULL);
		queue->tail->next = job;
		queue->tail = job;
	}
	pthread_rwlock_unlock(&queue->lock);		
}

//struct dtex_job*
//dtex_job_queue_find(struct dtex_job_queue* queue, pthread_t id)
//{
//	struct dtex_job* job;
//
//	if (pthread_rwlock_rdlock(&queue->lock) != 0)
//		return NULL;
//
//	for (job = queue->head; job != NULL; job = job->next) {
//		if (pthread_equal(job->id, id))
//			break;
//	}
//
//	pthread_rwlock_unlock(&queue->lock);
//	return job;
//}