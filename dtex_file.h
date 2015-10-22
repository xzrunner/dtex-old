#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_file_h
#define dynamic_texture_file_h

#include <stddef.h>
	
struct dtex_file;

struct dtex_file* dtex_file_open(const char* path, const char* format);
void dtex_file_close(struct dtex_file* f);

size_t dtex_file_size(struct dtex_file* f);

int dtex_file_read(struct dtex_file* f, void* buffer, size_t size);
int dtex_file_write(struct dtex_file* f, void* buffer, size_t size);

void dtex_file_seek_from_cur(struct dtex_file* f, int offset);
void dtex_file_seek_from_head(struct dtex_file* f, int offset);

#endif // dynamic_texture_file_h

#ifdef __cplusplus
}
#endif