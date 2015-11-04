#include "dtex_lod.h"

#include <string.h>

static int LOD[3] = {0, 0, 0};

void 
dtex_lod_init(int lod[3]) {
	memcpy(LOD, lod, sizeof(LOD));
}

float 
dtex_lod_get_scale(int lod) {
	float s = 1.0f;
	if (lod >= 0 && lod < 3) {
		s = LOD[lod] / 100.0f;
	}
	return s;
}