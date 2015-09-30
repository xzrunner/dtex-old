#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_relocation_h
#define dynamic_texture_relocation_h

#include <stdint.h>

/************************************************************************/
/* get src and dst trans position (rect to rect)                        */
/************************************************************************/
void dtex_prepare_c3_trans_pos(struct dtex_rect* src_rect, struct dtex_texture* src_tex, struct dtex_texture* dst_tex, 
                               struct dtex_img_pos* src_pos, struct dtex_img_pos* dst_pos);

/************************************************************************/
/* relocate pictures(quads) and textures                                */
/************************************************************************/
void dtex_relocate_spr(struct dtex_package* pkg, int tex_idx, struct dtex_array* pictures, 
                       struct dtex_img_pos* src, struct dtex_img_pos* dst);

/************************************************************************/
/* update c2 key                                                        */
/************************************************************************/
void dtex_relocate_c2_key(struct dtex_c2*, struct dtex_package* pkg, int tex_idx, 
                          struct dtex_array* pictures, struct dtex_img_pos* src, struct dtex_img_pos* dst);

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