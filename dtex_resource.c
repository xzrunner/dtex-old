#include "dtex_resource.h"
#include "dtex_typedef.h"

#include <string.h>
#include <stdlib.h>

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

	if (lod == 1) {
		strcat(ret, ".50.ept");
	} else if (lod == 2) {
		strcat(ret, ".25.ept");
	} else {
		strcat(ret, ".ept");
	} 

	ret[strlen(ret)] = 0;
}