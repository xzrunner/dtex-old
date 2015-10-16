#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_texture_h
#define dynamic_texture_texture_h

enum DTEX_TF_TYPE {
	DTEX_TT_INVALID = 0,
	DTEX_TT_RAW,
	DTEX_TT_MID
};

struct dtex_texture {
	unsigned int id;		// gl id

	int width, height;
	float inv_width, inv_height;

	int uid;				// id in dtex
							// >= QUAD_TEXID_IN_PKG_MAX

	int uid_3rd;			// id in other engine

	int type;
	union {
		struct {
			int id_alpha;	// for etc1
			int format;
			float scale;
			float lod_scale;
		} RAW;

		struct {
			struct dtex_tp* tp;
		} MID;
	} t;
};

struct dtex_texture* dtex_texture_create_raw(int lod);
struct dtex_texture* dtex_texture_create_mid(int edge);
void dtex_texture_release(struct dtex_texture*);

void dtex_texture_clear(struct dtex_texture*);

void dtex_texture_pool_init();

struct dtex_texture* dtex_texture_fetch(int uid);
unsigned int dtex_texture_get_gl_id(int uid);

#endif // dynamic_texture_texture_h

#ifdef __cplusplus
}
#endif