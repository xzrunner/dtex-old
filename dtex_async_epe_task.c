#include "dtex_async_epe_task.h"
#include "dtex_package.h"
#include "dtex_async_loader.h"
#include "dtex_desc_loader.h"

#include <stdlib.h>

struct params {
	void (*cb)(void* ud);
	void* ud;
};

static inline void
_load_epe_func(struct dtex_import_stream* is, void* ud) {
	struct params* params = (struct params*)ud;	

	// todo pkg id
//	struct dtex_package* pkg = dtex_package_create();
//	dtex_load_epe(is, pkg, 1, 0, 0);
	
	free(params);
}

void 
dtex_async_load_epe(const char* filepath, void (*cb)(void* ud), void* ud) {
	struct params* params = (struct params*)malloc(sizeof(*params));
	params->cb = cb;
	params->ud = ud;
	dtex_async_load_file(filepath, _load_epe_func, params, "epe");
}