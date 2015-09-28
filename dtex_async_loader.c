#include "dtex_async_loader.h"
#include "dtex_job_queue.h"
#include "dtex_loader_new.h"
#include "dtex_stream_import.h"
#include "dtex_typedef.h"
#include "dtex_package.h"
#include "dtex_texture_loader.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

static struct dtex_job_queue* load_queue;
static struct dtex_job_queue* parse_queue;

enum JOB_TYPE {
	JOB_INVALID = 0,
	JOB_LOAD_FILE,
	JOB_PARSER_DATA	
};

struct pkg_dst_params {
	int filetype;
	struct dtex_package* pkg;
	int idx;
	float scale;
};

struct load_file_params {
	char* filepath;
	struct pkg_dst_params dst;
};

struct parse_data_params {
// 	char* data;
// 	size_t size;
	struct dtex_import_stream is;
	struct pkg_dst_params dst;
};

void 
dtex_async_loader_init() {
	load_queue = dtex_job_queue_create();
	parse_queue = dtex_job_queue_create();
}

void 
dtex_async_loader_release() {
	dtex_job_queue_release(parse_queue);
	dtex_job_queue_release(load_queue);
}

static inline void
_unpack_memory_to_job(struct dtex_import_stream* is, void* ud) {
	struct parse_data_params* params = (struct parse_data_params*)malloc(sizeof(*params));

	size_t sz = is->size;
	char* buf = (char*)malloc(sz);
	memcpy(buf, is->stream, sz);

	params->is.size = sz;
	params->is.stream = buf;

	params->dst = *((struct pkg_dst_params*)ud);

	struct dtex_job* job = (struct dtex_job*)malloc(sizeof(*job));
	job->type = JOB_PARSER_DATA;
	job->ud = params;
	dtex_job_queue_push(parse_queue, job);
}

static inline void*
_load_file(void* arg) {
	struct dtex_job* job = dtex_job_queue_front_and_pop(load_queue);
	if (!job) {
		return NULL;
	}

	struct load_file_params* params = (struct load_file_params*)job->ud;
	dtex_load_file(params->filepath, &_unpack_memory_to_job, &params->dst);

	return NULL;
}

void 
dtex_async_load_file(const char* filepath, int filetype, struct dtex_package* pkg, int idx, float scale) {
	struct load_file_params* params = (struct load_file_params*)malloc(sizeof(*params));
	params->filepath = (char*)malloc(strlen(filepath) + 1);
	strcpy(params->filepath, filepath);
	params->filepath[strlen(filepath)] = 0;
	params->dst.filetype = filetype;
	params->dst.pkg = pkg;
	params->dst.idx = idx;
	params->dst.scale = scale;

	struct dtex_job* job = (struct dtex_job*)malloc(sizeof(*job));
	job->type = JOB_LOAD_FILE;
	job->ud = params;
	dtex_job_queue_push(load_queue, job);

	pthread_create(&job->id, NULL, _load_file, NULL);
}

void 
dtex_async_loader_update(struct dtex_buffer* buf) {
	struct dtex_job* job = dtex_job_queue_front_and_pop(parse_queue);
	if (!job) {
		return;
	}

	struct parse_data_params* params = (struct parse_data_params*)job->ud;
	struct pkg_dst_params* dst = &params->dst;
	if (dst->filetype == FILE_EPT) {
		struct dtex_raw_tex* tex = dst->pkg->textures[dst->idx];
		assert(tex);
		dtex_load_texture_all(buf, &params->is, tex);
	}
}