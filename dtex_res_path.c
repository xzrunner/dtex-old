#include "dtex_res_path.h"

#include <stdlib.h>
#include <string.h>

#define MAX_PATH_SIZE 512

struct dtex_res_path {
	char epe[MAX_PATH_SIZE];
	char ept[MAX_PATH_SIZE];

	int img_count;
	int lod_count;

	char images[1][MAX_PATH_SIZE];
};

struct dtex_res_path* 
dtex_res_path_create(int img_count, int lod_count) {
	size_t sz = sizeof(struct dtex_res_path) + sizeof(char) * MAX_PATH_SIZE * lod_count * img_count;
	struct dtex_res_path* rp = (struct dtex_res_path*)malloc(sz);
	memset(rp, 0, sz);

	rp->img_count = img_count;
	rp->lod_count = lod_count;

	return rp;
}

void 
dtex_res_path_release(struct dtex_res_path* rp) {
	free(rp);
}

void 
dtex_set_epe_filepath(struct dtex_res_path* rp, const char* path) {
	strcpy(rp->epe, path);
	rp->epe[strlen(rp->epe)] = 0;
}

const char* 
dtex_get_epe_filepath(struct dtex_res_path* rp) {
	return rp->epe;
}

void 
dtex_set_ept_filepath(struct dtex_res_path* rp, const char* path) {
	strcpy(rp->ept, path);
	rp->ept[strlen(rp->ept)] = 0;
}

const char* 
dtex_get_ept_filepath(struct dtex_res_path* rp) {
	return rp->ept;
}

void 
dtex_set_img_filepath(struct dtex_res_path* rp, const char* path, int idx, int lod) {
	char* img_path = &(rp->images[idx * rp->lod_count + lod][0]);
	strcpy(img_path, path);
	(img_path)[strlen(img_path)] = 0;
}

const char* 
dtex_get_img_filepath(struct dtex_res_path* rp, int idx, int lod) {
	return rp->images[idx * rp->lod_count + lod];
}