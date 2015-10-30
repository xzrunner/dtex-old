#include "dtex_timer_task.h"

#define MAX_TASK_COUNT 128

struct task {
	int time;

	void (*cb)(void* ud);
	void* ud;
};

static struct task TASKS[MAX_TASK_COUNT];
static int TASK_COUNT;

void 
dtex_timer_task_init() {
	TASK_COUNT = 0;
}

void 
dtex_timer_task_update() {
	for (int i = 0; i < TASK_COUNT; ) {
		struct task* t = &TASKS[i];
		if (--t->time <= 0) {
			t->cb(t->ud);
			TASKS[i] = TASKS[--TASK_COUNT];
		} else {
			++i;
		}
	}
}

void 
dtex_timer_task_init_add(int time, void (*cb)(void* ud), void* ud) {
	if (TASK_COUNT >= MAX_TASK_COUNT) {
		return;
	}

	struct task* t = &TASKS[TASK_COUNT++];
	t->time = time;
	t->cb = cb;
	t->ud = ud;
}