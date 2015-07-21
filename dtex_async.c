#include "dtex_async.h"
#include "dtex_loader.h"

#include <string.h>
#include <pthread.h>
#include <stdlib.h>

#define MAX_ARGS_SIZE 10
struct thread_args {
	void* args[MAX_ARGS_SIZE];
};

static inline void*
_load_epp(void* arguments) {
	struct thread_args* args = (struct thread_args*)arguments;

	struct dtex_loader* loader = (struct dtex_loader*)args->args[0];
	struct ej_package* pkg = (struct ej_package*)args->args[1];
	int* id = (int*)args->args[2];
	struct dtex_rect** rect = (struct dtex_rect**)args->args[3];
	char* path = (char*)args->args[4];

	dtexloader_load_spr2task(loader, pkg, rect, *id, path);

	free(id);
	free(rect);
	free(path);
	free(args);

	pthread_exit(NULL);
	return NULL;
}

void 
dtex_async_load_spr(struct dtex_loader* loader, struct ej_package* pkg, 
					struct dtex_rect** rect, int count, int id, const char* path) {
	pthread_t thread;
	struct thread_args* args = (struct thread_args*)malloc(sizeof(struct thread_args));
	args->args[0] = loader;
	args->args[1] = pkg;

	args->args[2] = malloc(sizeof(id));
	memcpy(args->args[2], &id, sizeof(id));

	struct dtex_rect** tmp_rect = malloc(sizeof(struct dtex_rect*) * count);
	for (int i = 0; i < count; ++i) {
		tmp_rect[i] = rect[i];
	}
	args->args[3] = tmp_rect;

	char* p = malloc(strlen(path) + 1);
	strcpy(p, path);
	args->args[4] = p;

	pthread_create(&thread, NULL, _load_epp, (void*)args);
}