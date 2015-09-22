#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_texture_pool_h
#define dynamic_texture_texture_pool_h

struct dtex_raw_tex {
	int id;
	int id_alpha;	// for etc1
	int width, height;
	// todo inv ?
	int format;

	char* filepath;	// cache for reopen
	int idx;		// in pool
};

void dtex_pool_init();

struct dtex_raw_tex* dtex_pool_add();
void dtex_pool_remove(struct dtex_raw_tex* tex);

struct dtex_raw_tex* dtex_pool_query(int id);

int dtex_pool_query_glid(int id);

#endif // dynamic_texture_texture_pool_h

#ifdef __cplusplus
}
#endif