//#include "dtex_loader.h"
//#include "dtex_rrp.h"
//#include "dtex_pts.h"
//#include "dtex_rrr.h"
//#include "dtex_b4r.h"
//#include "dtex_png.h"
//#include "dtex_pvr.h"
//#include "dtex_math.h"
//#include "dtex_gl.h"
//#include "dtex_etc1.h"
//#include "dtex_file.h"
//
//#include "spritepack.h"
//
//#include "LzmaDec.h"
//#ifdef _MSC_VER
//#include "Alloc.h"
//#else
//#include "LzAlloc.h"
//#endif // _MSC_VER
//
//#include <stdint.h>
//#include <stdlib.h>
//#include <stdio.h>
//#include <string.h>
//#include <assert.h>
//#include <pthread.h>
//
//// todo define max texture in one package
//#define MAX_TEX_PER_PKG 4
//
//#define PACKAGE_SIZE 512
//
//#define TASK_SIZE 64
//
//#define LZMA_PROPS_SIZE 5
//
//#ifdef _MSC_VER
//#define strdup _strdup
//#endif // _MSC_VER
//
//enum {
//	LT_NULL = 0,
//	LT_LOADED_BUF,
//	LT_LOADED_TEX,
//	LT_RELOCATED,
//	LT_FINISH
//};
//
//struct load_task {
//	bool is_epp;
//
//	struct ej_sprite_pack* ej_pkg;
//	struct dtex_rect* rect;
//	int spr_id;
//	int tex_idx;
//	struct dtex_raw_tex texture;
//
//	uint8_t* buf;
//	size_t sz;
//
//	int status;
//};
//
//struct task_list
//{
//	struct load_task* tasks[TASK_SIZE];
//	int capaticy;
//	int size;
//};
//
//struct dtex_loader {
//	struct dtex_package* packages[PACKAGE_SIZE];
//	int pkg_size;
//
//	// cache memory buf
//	uint8_t* buf;
//	size_t buf_size;
//	// cache tex id
//	// todo
//
//	struct task_list* task_list;
//};
//
//pthread_mutex_t mutexsum;
//
//struct task_list* _create_task_list() {
//	// todo �ڴ����
//	int capacity = TASK_SIZE;
//	struct task_list* list = (struct task_list*)malloc(sizeof(struct task_list));
//	list->capaticy = capacity;
//	list->size = 0;
//	for (int i = 0; i < capacity; ++i) {
//		struct load_task* task = malloc(sizeof(*task));
//		memset(task, 0, sizeof(*task));
//		task->tex_idx = -1;
//		list->tasks[i] = task;
//	}	
//	return list;
//}
//
//void _release_task_list(struct task_list* list) {
//	for (int i = 0; i < list->capaticy; ++i) {
//		free(list->tasks[i]);
//	}
//	free(list);
//}
//
//struct dtex_loader* 
//dtexloader_create() {
//	size_t sz = sizeof(struct dtex_loader);
//	struct dtex_loader* loader = (struct dtex_loader*)malloc(sz);
//	memset(loader, 0, sz);
//	loader->task_list = _create_task_list();
//
//	pthread_mutex_init(&mutexsum, NULL);
//
//	return loader;
//}
//
//static inline void
//_release_texture(struct dtex_raw_tex* tex) {
//	if (tex->id != 0) {
//		glActiveTexture(GL_TEXTURE0);
//		glBindTexture(GL_TEXTURE_2D, 0);
//		glDeleteTextures(1, &tex->id);
//		tex->id = 0;
//	}
//    if (tex->id_alpha != 0) {
//		glActiveTexture(GL_TEXTURE1);
//		glBindTexture(GL_TEXTURE_2D, 0);
//		glDeleteTextures(1, &tex->id_alpha);
//		tex->id_alpha = 0;
//    }
//}
//
//static inline void
//_release_package(struct dtex_package* pkg) {
//	free(pkg->ej_pkg); pkg->ej_pkg = NULL;
//	free(pkg->rrp_pkg); pkg->rrp_pkg = NULL;
//	free(pkg->pts_pkg); pkg->pts_pkg = NULL;
//	free(pkg->rrr_pkg); pkg->rrr_pkg = NULL;
//	free(pkg->b4r_pkg); pkg->b4r_pkg = NULL;
//
//	free(pkg->name); pkg->name = NULL;
//	free(pkg->filepath); pkg->filepath = NULL;
//
//	for (int i = 0; i < pkg->tex_size; ++i) {
//		_release_texture(&pkg->textures[i]);
//	}
//	pkg->tex_size = 0;
//
//	free(pkg);
//}
//
//void 
//dtexloader_release(struct dtex_loader* dtex) {
//	for (int i = 0; i < dtex->pkg_size; ++i) {
//		struct dtex_package* pkg = dtex->packages[i];
//		_release_package(pkg);
//	}
//
//	free(dtex->buf); dtex->buf = NULL;
//	dtex->buf_size = 0;
//
//	_release_task_list(dtex->task_list);
//
//	free(dtex);
//
//	pthread_mutex_destroy(&mutexsum);
//}
//
//static inline void
//_load_epp(uint8_t* buffer, size_t sz, struct dtex_package* pkg, int format) {
//	if (pkg->tex_size >= DTEX_PACK_TEX_SZ) {
//		dtex_fault("pkg->tex_size >= DTEX_PACK_TEX_SZ\n");
//	}
//
//	struct dtex_raw_tex* tex = &pkg->textures[pkg->tex_size++];
//	tex->id = tex->id_alpha = 0;
//	if (format == PVRTC) {
//		tex->width = buffer[1] | buffer[2] << 8;
//		tex->height = buffer[3] | buffer[4] << 8;
//	} else {
//		tex->width = buffer[0] | buffer[1] << 8;
//		tex->height = buffer[2] | buffer[3] << 8;		
//	}
//	tex->format = format;
//}
//
//static inline void
//_do_load_task(struct load_task* task) {
//	// only support epp now
//	if (!task->is_epp) {
//		return;
//	}
//
//	size_t sz = task->sz;
//	uint8_t* buf = task->buf;
//	int format = buf[0];
//
//	int width, height;
//	if (format == PVRTC) {
//		width = buf[2] | buf[3] << 8;
//		height = buf[4] | buf[5] << 8;
//	} else {
//		width = buf[1] | buf[2] << 8;
//		height = buf[3] | buf[4] << 8;
//	}
//
//	struct dtex_raw_tex* tex = &task->texture;
//	tex->id = tex->id_alpha = 0;
//	tex->width = width;
//	tex->height = height;
//	tex->format = format;
//	switch (format) {
//	case TEXTURE4: case TEXTURE8:
//		tex->id = _texture_create(buf+5, format, width, height);
//		break;
//	case PVRTC:
//		tex->id = _pvr_texture_create(buf+6, sz-6, buf[1], width, height);
//		break;
//    case PKMC:
//    	_etc1_texture_create(buf+5, width, height, &tex->id, &tex->id_alpha);
//		break;
//	default:
//		dtex_fault("Invalid package format %d\n",format);		
//	}
//
//	task->status = LT_LOADED_TEX;
//}
//
//struct unpack2pkg_params {
//	struct dtex_package* pkg;
//	int load_tex_idx;
//};
//
//static inline void
//_unpack_memory_to_pkg(int block_idx, uint8_t* buffer, size_t sz, void* ud) {
//	struct unpack2pkg_params* params = (struct unpack2pkg_params*)ud;
//
//	int format = buffer[0];
//	if (params->load_tex_idx >= 0) {
//		struct dtex_raw_tex* tex = &params->pkg->textures[params->load_tex_idx];
//		assert(tex->format == format);
//		switch (format) {
//		case TEXTURE4: case TEXTURE8:
//			_load_png(buffer+1, format, tex);
//			break;
//		case PVRTC:
//			_load_pvr(buffer+1, sz-1, tex);
//			break;
//		case PKMC:
//			_load_etc1(buffer+1, tex);
//			break;
//		default:
//			dtex_fault("Invalid package format %d\n",format);
//		}
//		++params->load_tex_idx;
//	} else {
//		switch (format) {
//		case TEXTURE4: case TEXTURE8: case PVRTC: case PKMC:
//			_load_epp(buffer+1, sz-1, params->pkg, format);
//			break;
//		case DETAIL:
//			_load_epd(buffer+1, sz-1, params->pkg);
//			break;
//		case RRP:
//			_load_rrp(buffer+1, sz-1, params->pkg);
//			break;
//		case PTS:
//			_load_pts(buffer+1, sz-1, params->pkg);
//			break;
//		case RRR:
//			_load_rrr(buffer+1, sz-1, params->pkg);
//			break;
//		case B4R:
//			_load_b4r(buffer+1, sz-1, params->pkg);
//			break;
//		default:
//			dtex_fault("Invalid package format %d\n",format);
//		}
//	}
//}
//
//struct unpack2task_params {
//	struct dtex_loader* dtex;
//
//	bool is_epp;
//
//	struct ej_sprite_pack* ej_pkg;	
//	struct dtex_rect** rect;
//	int spr_id;
//};
//
//static inline void
//_unpack_memory_to_task(int block_idx, uint8_t* buffer, size_t sz, void* ud) {
//	struct unpack2task_params* params = (struct unpack2task_params*)ud;
//
//	struct task_list* tl = params->dtex->task_list;
//	assert(tl->size < tl->capaticy);
//	struct load_task* task = tl->tasks[tl->size++];
//
//	task->status = LT_NULL;
//
//	task->is_epp = params->is_epp;
//	task->ej_pkg = params->ej_pkg;
//	task->rect = params->rect[block_idx];
//	task->spr_id = params->spr_id;
//	task->tex_idx = block_idx;
//
//	if (task->sz == 0 || sz > task->sz) {
////        printf("alloc buf\n");
//		free(task->buf);
//		task->buf = (uint8_t*)malloc(sz);
//	}
//	task->sz = sz;
//	memcpy(task->buf, buffer, sz);
//	task->status = LT_LOADED_BUF;
//}
//
//static inline void
//_buf_reserve(struct dtex_loader* dtex, uint32_t sz) {
//	if (sz <= dtex->buf_size) {
//		return;
//	}
//
//	int new_sz = sz * 1.5f;
//	unsigned char* buf = malloc(new_sz);
//	if (buf == NULL) {
//		return;
//	}
//	dtex->buf_size = new_sz;
//	free(dtex->buf);
//	dtex->buf = buf;
//
//	dtex_info("dtex_loader buf size:%0.1fM\n", (float)new_sz / 1024 / 1024);
//}
//
//static inline int 
//_check_block_count(struct dtex_file* file) {
//	int count = 0;
//	for (;;) {
//		int32_t sz = 0;
//		if (dtex_file_read(file, &sz, sizeof(sz)) == 0) {
//			return count;
//		}
//		++count;
//		if (sz < 0) {
//			sz = -sz;
//		}
//		dtex_file_seek_from_cur(file, sz);
//	}
//}
//
//#define LZMA_PROPS_SIZE 5
//
//static void *SzAlloc(void *p, size_t size) { p = p; return MyAlloc(size); }
//static void SzFree(void *p, void *address) { p = p; MyFree(address); }
//static ISzAlloc g_Alloc = { SzAlloc, SzFree };
//
//static inline int
//_lzma_uncompress(unsigned char *dest, size_t  *destLen, const unsigned char *src, size_t  *srcLen, 
//	const unsigned char *props, size_t propsSize)
//{
//	ELzmaStatus status;
//	return LzmaDecode(dest, destLen, src, srcLen, props, (unsigned)propsSize, LZMA_FINISH_ANY, &status, &g_Alloc);
//}
//
//struct block {
//	uint8_t size[4];
//	uint8_t prop[LZMA_PROPS_SIZE];
//	uint8_t data[119];
//};
//
//static inline void
//_unpack_file(struct dtex_loader* dtex, struct dtex_file* file, void (*unpack_func)(), void* ud) {
//	int block_count = _check_block_count(file);
//	dtex_file_seek_from_head(file, 0);
// 
//	for (int i = 0; i < block_count; ++i) {
//		int32_t sz = 0;
//		dtex_file_read(file, &sz, sizeof(sz));
//		if (sz < 0) {
//			sz = -sz;
//			_buf_reserve(dtex, sz);
//			if (dtex_file_read(file, dtex->buf, sz) != 1) {
//				dtex_fault("Invalid uncompress data source\n");
//			}
//			unpack_func(i, dtex->buf, sz, ud);
//		} else {
//			uint8_t ori_sz_arr[4];
//			dtex_file_read(file, ori_sz_arr, sizeof(ori_sz_arr));
//			dtex_file_seek_from_cur(file, -sizeof(ori_sz_arr));
//			size_t ori_sz = ori_sz_arr[0] << 24 | ori_sz_arr[1] << 16 | ori_sz_arr[2] << 8 | ori_sz_arr[3];
//			size_t need_sz = sz + 7 + ori_sz;
//			_buf_reserve(dtex, need_sz);
//
//			struct block* block = (struct block*)dtex->buf;
//			if (sz <= 4 + LZMA_PROPS_SIZE || dtex_file_read(file, block, sz) != 1) {
//				dtex_fault("Invalid compress data source\n");
//			}
//
//			unsigned char* buffer = (unsigned char*)(dtex->buf + ((sz + 3) & ~3));
//			size_t compressed_sz = sz - sizeof(block->size) - LZMA_PROPS_SIZE;
//
//			int r = _lzma_uncompress(buffer, &ori_sz, block->data, &compressed_sz, block->prop, LZMA_PROPS_SIZE);
//			if (r != SZ_OK) {
//				dtex_fault("Uncompress error %d\n",r);
//			}
//			unpack_func(i, buffer, ori_sz, ud);		
//		}
//	}
//}
//
//static inline struct dtex_package**
//_find_package(struct dtex_loader* dtex, const char* name) {
//	for (int i = 0; i < dtex->pkg_size; ++i) {
//		struct dtex_package** pkg = &dtex->packages[i];
//		if (strcmp(name, (*pkg)->name) == 0) {
//			return pkg;
//		}
//	}
//	return NULL;
//}
//
//static inline struct dtex_package*
//_new_package(struct dtex_loader* dtex, const char* name) {
//	struct dtex_package* pkg = (struct dtex_package*)malloc(sizeof(struct dtex_package));
//	memset(pkg, 0, sizeof(*pkg));
//	pkg->name = strdup(name);
//	pkg->filepath = NULL;
//	pkg->tex_size = 0;
//	pkg->tex_scale = 1;
//
//	if (dtex->pkg_size >= PACKAGE_SIZE) {
//		dtex_fault("dtex->pack_size >= PACKAGE_SIZE\n");
//	}		
//	dtex->packages[dtex->pkg_size++] = pkg;
//	return pkg;
//}
//
//struct dtex_package* 
//dtexloader_preload_pkg(struct dtex_loader* dtex, const char* name, const char* path) {
//	// open file
//	struct dtex_file* file = dtex_file_open(path, "rb");
//	if (file == NULL) {
//		dtex_fault("Can't open name: %s file: %s\n", name, path);
//	}
//
//	struct dtex_package* pkg = NULL;
//	struct dtex_package** pkg_find = _find_package(dtex, name);
//	if (pkg_find == NULL) {
//		pkg = _new_package(dtex, name);
//	} else if (*pkg_find == NULL) {
//		pkg = *pkg_find = _new_package(dtex, name);
//	} else {
//		pkg = *pkg_find;
//	}
//
//	int old_tex_size = pkg->tex_size;
//
//	struct unpack2pkg_params params;
//	params.pkg = pkg;
//	params.load_tex_idx = -1;
//	_unpack_file(dtex, file, &_unpack_memory_to_pkg, &params);
//
//	dtex_file_close(file);
//
//	// if load epp
//	if (pkg->tex_size > old_tex_size && pkg->filepath == NULL) {
//		pkg->filepath = strdup(path);
//	}
//
//	return pkg;
//}
//
//static inline void
//_unload_texture(struct dtex_raw_tex* tex) {
//	glDeleteTextures(1, &tex->id); tex->id = 0;
//	if (tex->id_alpha != 0) {
//		glDeleteTextures(1, &tex->id_alpha); tex->id_alpha = 0;		
//	}
//}
//
//static inline void 
//_unload_package(struct dtex_package* pkg) {
//	for (int i = 0; i < pkg->tex_size; ++i) {
//		_unload_texture(&pkg->textures[i]);
//	}
//}
//
//struct dtex_raw_tex* 
//dtexloader_load_epp(struct dtex_loader* dtex, struct dtex_package* pkg, int tex_idx) {
//	static struct dtex_package* last_pack = NULL;
//	assert(pkg != NULL && tex_idx < pkg->tex_size && tex_idx >= 0);
//	struct dtex_raw_tex* tex = &pkg->textures[tex_idx];
//	if (tex->id == 0) {
//		if (pkg != last_pack && last_pack != NULL) {
//			_unload_package(last_pack);
//			last_pack = NULL;
//		}
//		struct dtex_file* file = dtex_file_open(pkg->filepath, "rb");
//		if (file == NULL) {
//			dtex_fault("Can't open name: %s file: %s\n", pkg->name, pkg->filepath);
//		}
//		struct unpack2pkg_params params;
//		params.pkg = pkg;
//		params.load_tex_idx = 0;
//		_unpack_file(dtex, file, &_unpack_memory_to_pkg, &params);		
//		last_pack = pkg;
//	}
//	assert(tex->id != 0);
//	return tex;	
//}
//
//struct dtex_raw_tex* 
//dtexloader_load_image(const char* path) {
//	if (strstr(path, ".png")) {
//		int w, h, c, f;
//		uint8_t* p = dtex_png_read(path, &w, &h, &c, &f);
//
//		GLuint tex = dtex_prepare_texture(GL_TEXTURE0);
//		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)w, (GLsizei)h, 0, GL_RGBA, GL_UNSIGNED_BYTE, p);
//
//		free(p);
//
//		struct dtex_raw_tex* ret = (struct dtex_raw_tex*)malloc(sizeof(struct dtex_raw_tex));
//		ret->format = TEXTURE8;
//		ret->width = w;
//		ret->height = h;
//		ret->id = tex;
//		ret->id_alpha = 0;
//		return ret;
//	} else {
//		return NULL;
//	}
//}
//
//void 
//dtexloader_unload_tex(struct dtex_raw_tex* tex) {
//	_release_texture(tex);
//}
//
//struct dtex_package* 
//dtexloader_get_pkg(struct dtex_loader* dtex, int idx) {
//	if (idx < 0 || idx >= dtex->pkg_size) {
//		return NULL;
//	} else {
//		return dtex->packages[idx];
//	}
//}
//
//bool 
//dtexloader_has_task(struct dtex_loader* dtex) {
//	return dtex->task_list->size != 0;
//}
//
//// todo if already exists
//void 
//dtexloader_load_spr2task(struct dtex_loader* dtex, struct ej_sprite_pack* ej_pkg, struct dtex_rect** rect, int id, const char* path) {
//	struct dtex_file* file = dtex_file_open(path, "rb");
//	if (file == NULL) {
//		dtex_fault("[dtexloader_load_spr2task] Can't open file: %s\n", path);
//	}
//
//	pthread_mutex_lock (&mutexsum);
//
//	struct unpack2task_params params;
//	params.dtex = dtex;
//	params.is_epp = true;	
//	params.ej_pkg = ej_pkg;
//	params.rect = rect;
//	params.spr_id = id;
//
//	_unpack_file(dtex, file, &_unpack_memory_to_task, &params);
//
//	dtex_file_close(file);
//
//	pthread_mutex_unlock (&mutexsum);
//}
//
//// to c2
//void 
//dtexloader_do_task(struct dtex_loader* dtex, void (*on_load_func)()) {
//	pthread_mutex_lock (&mutexsum);
//
//	struct task_list* tl = dtex->task_list;
//	for (int i = 0; i < tl->size; ++i) {
//		struct load_task* task = tl->tasks[i];
//		if (task->status != LT_LOADED_BUF) {
//			continue;
//		}
//		// load task
//		_do_load_task(task);
//
//		// parser task data
//		on_load_func(task->ej_pkg, task->rect, task->spr_id, task->tex_idx, &task->texture);
//		task->status = LT_RELOCATED;
//
//		// release task
//		free(task->buf); task->buf = NULL;
//		task->sz = 0;
//	}
//
//	pthread_mutex_unlock (&mutexsum);
//}
//
//void 
//dtexloader_after_do_task(struct dtex_loader* dtex, void (*after_load_func)()) {
//	pthread_mutex_lock (&mutexsum);
//
//	struct task_list* tl = dtex->task_list;
//
//	//////////////////////////////////////////////////////////////////////////
//	//for (int i = 0; i < tl->free_idx; ++i) {
//	//	struct load_task* task = tl->tasks[i];
//
//	//	if (task->status != LT_RELOCATED) {
//	//		continue;
//	//	}
//
//	//	after_load_func(task->pkg, task->rect, task->spr_id, task->tex_idx, &task->texture);
//	//	_release_texture(&task->texture);
//
//	//	task->status = LT_FINISH;		
//	//}
//	//////////////////////////////////////////////////////////////////////////
//
//	int curr_idx = 0;
//	while (curr_idx != tl->size)
//	{
//		struct load_task* task = tl->tasks[curr_idx];
//
//		if (task->status != LT_RELOCATED) {
//			++curr_idx;
//			continue;
//		}
//
//		after_load_func(task->ej_pkg, task->rect, task->spr_id, task->tex_idx, &task->texture);
//		_release_texture(&task->texture);
//
//		task->status = LT_FINISH;		
//		task->status = -1;
//
//		assert(tl->size != 0);
//		if (curr_idx + 1 == tl->size) {
//			--tl->size;
//			break;
//		} else {
//			struct load_task* tmp = tl->tasks[curr_idx];
//			tl->tasks[curr_idx] = tl->tasks[--tl->size];
//			tl->tasks[tl->size] = tmp;
//		}
//	}
//
//	//////////////////////////////////////////////////////////////////////////
//
//	pthread_mutex_unlock (&mutexsum);
//}
//
//struct dtex_rrp* 
//dtexloader_query_rrp(struct dtex_loader* dtex, struct ej_sprite_pack* ej_pkg) {
//    if (!dtex) {
//        return NULL;
//    }
//    
//	static struct dtex_package* cache_pkg = NULL;
//	if (cache_pkg && cache_pkg->ej_pkg == ej_pkg) {
//		return cache_pkg->rrp_pkg;
//	}
//
//	for (int i = 0; i < dtex->pkg_size; ++i) {
//		struct dtex_package* pkg = dtex->packages[i];
//		// todo
//		// if (pkg->ej_pkg == ej_pkg) {
//			cache_pkg = pkg;
//			return pkg->rrp_pkg;
//		// }
//	}
//	return NULL;
//}
//
//struct dtex_pts* 
//dtexloader_query_pts(struct dtex_loader* dtex, struct ej_sprite_pack* ej_pkg) {
//	static struct dtex_package* cache_pkg = NULL;
//	if (cache_pkg && cache_pkg->ej_pkg == ej_pkg) {
//		return cache_pkg->pts_pkg;
//	}
//
//	for (int i = 0; i < dtex->pkg_size; ++i) {
//		struct dtex_package* pkg = dtex->packages[i];
//		// todo
//		// if (pkg->ej_pkg == ej_pkg) {
//			cache_pkg = pkg;
//			return pkg->pts_pkg;
//		// }
//	}
//	return NULL;
//}