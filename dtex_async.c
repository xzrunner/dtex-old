#include "dtex_async.h"
#include "dtex_loader.h"

#include <string.h>
#include <pthread.h>

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
	struct dtex_rect* rect = (struct dtex_rect*)args->args[3];
	const char* path = (const char*)args->args[4];

	dtexloader_load_spr2task(loader, pkg, rect, *id, path);
//	pthread_exit(NULL);
	return NULL;
}

void 
dtex_async_load_spr(struct dtex_loader* loader, struct ej_package* pkg, struct dtex_rect* rect, int id, const char* path) {
	pthread_t thread;
	struct thread_args args;
	args.args[0] = loader;
	args.args[1] = pkg;
	args.args[2] = &id;
	args.args[3] = rect;

	char p[strlen(path)];
	strcpy(p, path);
	args.args[4] = p;

	pthread_create(&thread, NULL, _load_epp, (void*)&args);
}