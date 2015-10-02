#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_relocation_h
#define dynamic_texture_relocation_h

#include "dtex_typedef.h"

#include <stdint.h>

struct dtex_texture_with_rect {
	struct dtex_texture* tex;
	struct dtex_rect rect;
//	bool is_rotated;
};

/************************************************************************/
/* swap quad's texid and texcoords with extend info of pkg.             */
/************************************************************************/
void dtex_swap_quad_src_info(struct dtex_package* pkg, struct dtex_array* picture_ids);

/************************************************************************/
/* relocate pictures(quads) and textures                                */
/************************************************************************/
void dtex_relocate_spr(struct dtex_package* pkg, int tex_idx, struct dtex_array* pictures, 
                       struct dtex_texture_with_rect* src, struct dtex_texture_with_rect* dst);

/************************************************************************/
/* update c2 key, origin key (pkg) to c3                                */
/************************************************************************/
void dtex_relocate_c2_key(struct dtex_c2*, struct dtex_package* pkg, int tex_idx, 
                          struct dtex_array* pictures, struct dtex_texture_with_rect* src, struct dtex_texture_with_rect* dst);

/************************************************************************/
/* trans_vb for draw, dst_vb is texcoords                               */
/************************************************************************/
void dtex_relocate_quad(uint16_t part_src[8], struct dtex_inv_size* src_sz, struct dtex_rect* src_rect, 
                        struct dtex_inv_size* dst_sz, struct dtex_rect* dst_rect, int rotate, float trans_vb[16], float dst_vb[8]);

/************************************************************************/
/* get quad texcoords region                                            */
/************************************************************************/
void dtex_get_texcoords_region(uint16_t* texcoords, struct dtex_rect* region);

#endif // dynamic_texture_relocation_h

#ifdef __cplusplus
}
#endif