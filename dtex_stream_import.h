#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_stream_import_h
#define dynamic_texture_stream_import_h

#include <stdint.h>
#include <stddef.h>
	
struct dtex_import_stream {
	const char* stream;
	size_t size;
};

uint8_t
dtex_import_uint8(struct dtex_import_stream* is);

uint16_t
dtex_import_uint16(struct dtex_import_stream* is);

uint32_t
dtex_import_uint32(struct dtex_import_stream* is);

const char*
dtex_import_string(struct dtex_import_stream* is);

#endif // dynamic_texture_stream_import_h

#ifdef __cplusplus
}
#endif