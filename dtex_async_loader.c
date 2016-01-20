#include "dtex_async_loader.h"
#include "dtex_async_queue.h"
#include "dtex_loader.h"
#include "dtex_stream_import.h"

#include "dtex_log.h"

#include <pthread.h>

#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct job {
	struct job* next;

	pthread_t id;
	int type;
	void* ud;

	char desc[32];
};

struct job_queue {
 	struct job* head;
 	struct job* tail;
 	pthread_rwlock_t lock;
};

static struct job_queue JOB_FREE_QUEUE;
static struct job_queue JOB_LOAD_QUEUE;
static struct job_queue JOB_PARSE_QUEUE;

static int VERSION;
static pthread_mutex_t VERSION_LOCK;

enum JOB_TYPE {
	JOB_INVALID = 0,
	JOB_LOAD_FILE,
	JOB_PARSER_DATA	
};

struct load_params {
	int version;

	struct load_params* next;

	char filepath[512];
	char desc[32];
	void (*cb)(struct dtex_import_stream* is, void* ud);
	void* ud;
};

struct load_params_queue {
	struct load_params* head;
	struct load_params* tail;
	pthread_rwlock_t lock;
};

static struct load_params_queue PARAMS_LOAD_QUEUE;

struct parse_params {
	int version;

	struct parse_params* next;

	char* data;
	size_t size;
	void (*cb)(struct dtex_import_stream* is, void* ud);
	void* ud;
};

struct parse_params_queue {
	struct parse_params* head;
	struct parse_params* tail;
	pthread_rwlock_t lock;
};

static struct parse_params_queue PARAMS_PARSE_QUEUE;

void 
dtex_async_loader_init() {
	DTEX_ASYNC_QUEUE_INIT(JOB_FREE_QUEUE);
	DTEX_ASYNC_QUEUE_INIT(JOB_LOAD_QUEUE);
	DTEX_ASYNC_QUEUE_INIT(JOB_PARSE_QUEUE);
	DTEX_ASYNC_QUEUE_INIT(PARAMS_LOAD_QUEUE);
	DTEX_ASYNC_QUEUE_INIT(PARAMS_PARSE_QUEUE);

	VERSION = 0;
	pthread_mutex_init(&VERSION_LOCK, 0);
}

void 
dtex_async_loader_release() {
	dtex_async_loader_clear();
}

void
dtex_async_loader_clear() {
	pthread_mutex_lock(&VERSION_LOCK);
	++VERSION;

	struct job* job = NULL;

	do {
		DTEX_ASYNC_QUEUE_POP(JOB_LOAD_QUEUE, job);
		if (job) {
			dtex_debug_to_file("async_load clear pop JOB_LOAD_QUEUE, job: %p", job);
			DTEX_ASYNC_QUEUE_PUSH(JOB_FREE_QUEUE, job);
		}
	} while (job);

	do {
		DTEX_ASYNC_QUEUE_POP(JOB_PARSE_QUEUE, job);
		if (job) {
			dtex_debug_to_file("async_load clear pop JOB_PARSE_QUEUE, job: %p", job);
			struct parse_params* params = (struct parse_params*)job->ud;
			free(params->data), params->data = NULL;
			params->size = 0;
			DTEX_ASYNC_QUEUE_PUSH(PARAMS_PARSE_QUEUE, params);
			DTEX_ASYNC_QUEUE_PUSH(JOB_FREE_QUEUE, job);
		}
	} while (job);

	pthread_mutex_unlock(&VERSION_LOCK);
}

static inline int
_get_version() {
	int ret;
	pthread_mutex_lock(&VERSION_LOCK);
	ret = VERSION;
	pthread_mutex_unlock(&VERSION_LOCK);
	return ret;
}

static inline bool
_is_valid_version(int version) {
	bool ret;
	pthread_mutex_lock(&VERSION_LOCK);
	ret = version == VERSION;
	pthread_mutex_unlock(&VERSION_LOCK);
	return ret;
}

static inline void
_unpack_memory_to_job(struct dtex_import_stream* is, void* ud) {
	struct load_params* prev_params = (struct load_params*)ud;
	if (!_is_valid_version(prev_params->version)) {
		return;
	}

	struct parse_params* params = NULL;
	DTEX_ASYNC_QUEUE_POP(PARAMS_PARSE_QUEUE, params);
	if (!params) {
		params = (struct parse_params*)malloc(sizeof(*params));
	}

	params->version = _get_version();

	size_t sz = is->size;
	char* buf = (char*)malloc(sz);
	memcpy(buf, is->stream, sz);

	params->size = sz;
	params->data = buf;

	params->cb = prev_params->cb;
	params->ud = prev_params->ud;

	struct job* job = NULL;
	DTEX_ASYNC_QUEUE_POP(JOB_FREE_QUEUE, job);
	if (!job) {
		job = (struct job*)malloc(sizeof(*job));
	}
	job->type = JOB_PARSER_DATA;
	job->ud = params;
	memcpy(job->desc, prev_params->desc, sizeof(prev_params->desc));
	DTEX_ASYNC_QUEUE_PUSH(JOB_PARSE_QUEUE, job);

	dtex_debug_to_file("async_load push 3, job: %p", job);
}

static inline void*
_load_file(void* arg) {
	struct job* job = NULL;
	DTEX_ASYNC_QUEUE_POP(JOB_LOAD_QUEUE, job);
	if (!job) {
		return NULL;
	}

	dtex_debug_to_file("async_load pop 2, job: %p", job);

	struct load_params* params = (struct load_params*)job->ud;
	if (_is_valid_version(params->version)) {
		memcpy(params->desc, job->desc, sizeof(job->desc));
		dtex_load_file(params->filepath, &_unpack_memory_to_job, params);
	}

	DTEX_ASYNC_QUEUE_PUSH(PARAMS_LOAD_QUEUE, params);
	DTEX_ASYNC_QUEUE_PUSH(JOB_FREE_QUEUE, job);

	return NULL;
}

void 
dtex_async_load_file(const char* filepath, void (*cb)(struct dtex_import_stream* is, void* ud), void* ud, const char* desc) {
	struct load_params* params = NULL;
	DTEX_ASYNC_QUEUE_POP(PARAMS_LOAD_QUEUE, params);
	if (!params) {
		params = (struct load_params*)malloc(sizeof(*params));
	}

	params->version = _get_version();

	strcpy(params->filepath, filepath);
	params->filepath[strlen(filepath)] = 0;

	params->cb = cb;
	params->ud = ud;

	struct job* job = NULL;
	DTEX_ASYNC_QUEUE_POP(JOB_FREE_QUEUE, job);
	if (!job) {
		job = (struct job*)malloc(sizeof(*job));
	}

	job->type = JOB_LOAD_FILE;
	job->ud = params;
	strcpy(job->desc, desc);
	job->desc[strlen(job->desc)] = 0;
	DTEX_ASYNC_QUEUE_PUSH(JOB_LOAD_QUEUE, job);

	dtex_debug_to_file("async_load push 1, job: %p", job);

	pthread_create(&job->id, NULL, _load_file, NULL);
}

void 
dtex_async_loader_update() {
	struct job* job = NULL;
	DTEX_ASYNC_QUEUE_POP(JOB_PARSE_QUEUE, job);
	if (!job) {
		return;
	}

	dtex_debug_to_file("async_load pop 4, job: %p", job);

	struct parse_params* params = (struct parse_params*)job->ud;
	if (_is_valid_version(params->version) && params->cb) {
		struct dtex_import_stream is;
		is.stream = params->data;
		is.size = params->size;
		params->cb(&is, params->ud);
	}

	free(params->data), params->data = NULL;
	params->size = 0;
	DTEX_ASYNC_QUEUE_PUSH(PARAMS_PARSE_QUEUE, params);
	DTEX_ASYNC_QUEUE_PUSH(JOB_FREE_QUEUE, job);
}

bool 
dtex_async_loader_empty() {
	return DTEX_ASYNC_QUEUE_EMPTY(JOB_LOAD_QUEUE)
		&& DTEX_ASYNC_QUEUE_EMPTY(JOB_PARSE_QUEUE);
}