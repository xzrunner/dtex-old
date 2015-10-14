#include "dtex_resource.h"
#include "dtex_typedef.h"

#include <string.h>
#include <stdlib.h>

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

void 
dtex_get_resource_filepath(const char* filepath, int format, char* ret) {
	strcpy(ret, filepath);

	if (format == FILE_EPE) {
		strcat(ret, ".epe");
	} else if (format == FILE_RRP) {
		strcat(ret, ".rrp");
	} else if (format == FILE_PTS) {
		strcat(ret, ".pts");
	} else if (format == FILE_RRR) {
		strcat(ret, ".rrr");
	} else if (format == FILE_B4R) {
		strcat(ret, ".b4r");
	}

	ret[strlen(ret)] = 0;
}

void 
dtex_get_texture_filepath(const char* filepath, int idx, int lod, char* ret) {
	strcpy(ret, filepath);

	strcat(ret, ".");

	char idx_str[3];
	strcat(ret, itoa(idx + 1, idx_str, 10));

	char lod_str[10];
	if (lod == 1) {
		strcat(ret, ".");
		itoa(LOD[1], lod_str, 10);
		strcat(ret, lod_str);
	} else if (lod == 2) {
		strcat(ret, ".");
		itoa(LOD[2], lod_str, 10);
		strcat(ret, lod_str);
	}
	strcat(ret, ".ept");

	ret[strlen(ret)] = 0;
}