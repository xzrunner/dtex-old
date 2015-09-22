#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_desc_loader_h
#define dynamic_texture_desc_loader_h

#include "ejoy2d.h"

struct dtex_rrp;
struct dtex_pts;
struct dtex_rrr;
struct dtex_b4r;

struct ej_sprite_pack* dtex_load_epe(uint8_t* buf);

struct dtex_rrp* dtex_load_rrp(uint8_t* buf, size_t sz);

struct dtex_pts* dtex_load_pts(uint8_t* buf, size_t sz);

struct dtex_rrr* dtex_load_rrr(uint8_t* buf, size_t sz);

struct dtex_b4r* dtex_load_b4r(uint8_t* buf, size_t sz);

#endif // dynamic_texture_desc_loader_h

#ifdef __cplusplus
}
#endif