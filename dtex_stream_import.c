#include "dtex_stream_import.h"
#include "dtex_log.h"

#include <stdlib.h>
#include <string.h>

#define STREAM_IMPORT(is, data)						\
	memcpy(&(data), (is)->stream, sizeof(data));	\
	(is)->stream += sizeof(data);					\
	(is)->size -= sizeof(data);

uint8_t
dtex_import_uint8(struct dtex_import_stream* is) {
	if (is->size < 1) {
		dtex_fault("dtex_import_uint8");
	}
	uint8_t ret;
	STREAM_IMPORT(is, ret);
	return ret;
}

uint16_t
dtex_import_uint16(struct dtex_import_stream* is) {
	if (is->size < 2) {
		dtex_fault("dtex_import_uint16");
	}
	uint16_t ret;
	STREAM_IMPORT(is, ret);
	return ret;
}

uint32_t
dtex_import_uint32(struct dtex_import_stream* is) {
	if (is->size < 4) {
		dtex_fault("dtex_import_uint32");
	}
	uint32_t ret;
	STREAM_IMPORT(is, ret);
	return ret;
}

const char*
dtex_import_string(struct dtex_import_stream* is) {
	uint8_t n = dtex_import_uint8(is);
	if (n == 255) {
		return NULL;
	}
	if (is->size < n) {
		dtex_fault("dtex_import_string");
	}

	char* buf = (char*)malloc(n + 1);
	memcpy(buf, is->stream, n);
	buf[n] = 0;
	is->stream += n;
	is->size -= n;
	return buf;
}
