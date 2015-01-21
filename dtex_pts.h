#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_picture_triangle_strip_h
#define dynamic_texture_picture_triangle_strip_h

#include <stdint.h>

#define EXPORT_PTS

struct pts_strip {
	int16_t pos_size;
	int16_t* vertices;
};

struct pts_picture {
	int16_t id;

	int16_t strip_size;
	struct pts_strip* strips;
};

struct dtex_pts;

struct dtex_pts* dtex_pts_create(void* data, int sz, int cap);
void dtex_pts_release(struct dtex_pts*);

struct pts_picture* dtex_pts_get_pic(struct dtex_pts*, int id);

#ifdef EXPORT_PTS
size_t dtex_pts_size(void* data, int sz);
#endif // EXPORT_PTS

#endif // dynamic_texture_picture_triangle_strip_h

#ifdef __cplusplus
}
#endif