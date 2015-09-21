#include "dtex_file.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct dtex_file {
	FILE* fp;
};

struct dtex_file* 
dtex_file_open(const char* path, const char* format) {
	FILE* fp = fopen(path, format);
	if (fp) {
		struct dtex_file* ret = malloc(sizeof(*ret));
		ret->fp = fp;
		return ret;
	} else {
		return NULL;
	}
}

void 
dtex_file_close(struct dtex_file* f) {
	if (f == NULL) return;

	if (f->fp) {
		fclose(f->fp);
	}
	free(f);
}

size_t 
dtex_file_size(struct dtex_file* f) {
	size_t save_pos = ftell(f->fp);
	fseek(f->fp, 0, SEEK_END);
	size_t sz = ftell(f->fp);
	fseek(f->fp, save_pos, SEEK_SET);
	return sz;
}

int 
dtex_file_read(struct dtex_file* f, void* buffer, size_t size) {
	return fread(buffer, size, 1, f->fp);
}

int 
dtex_file_write(struct dtex_file* f, void* buffer, size_t size) {
	return fwrite(buffer, size, 1, f->fp);
}

void 
dtex_file_seek_from_cur(struct dtex_file* f, int offset) {
	fseek(f->fp, offset, SEEK_CUR);
}

void
dtex_file_seek_from_head(struct dtex_file* f, int offset) {
	fseek(f->fp, offset, SEEK_SET);
}
