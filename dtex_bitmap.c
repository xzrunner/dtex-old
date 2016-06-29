#include "dtex_bitmap.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

uint8_t* 
dtex_bmp_init_blank(int edge) {
	size_t sz = edge * edge * 4;
	uint8_t* buf = (uint8_t*)malloc(sz);
	memset(buf, 0, sz);
	return buf;
}

void
dtex_bmp_revert_y(uint32_t* data, int width, int height) {
	uint32_t buf[width];
	int line_sz = width * sizeof(uint32_t);
	int bpos = 0, epos = width * (height - 1);
	for (int i = 0, n = floor(height / 2); i < n; ++i) {
		memcpy(buf, &data[bpos], line_sz);
		memcpy(&data[bpos], &data[epos], line_sz);
		memcpy(&data[epos], buf, line_sz);
		bpos += width;
		epos -= width;
	}
}