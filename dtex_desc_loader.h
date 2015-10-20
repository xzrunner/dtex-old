#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_desc_loader_h
#define dynamic_texture_desc_loader_h

#include "ejoy2d.h"

struct dtex_import_stream;

struct dtex_rrp;
struct dtex_pts;
struct dtex_rrr;
struct dtex_b4r;

void dtex_load_epe(struct dtex_import_stream* is, struct dtex_package* pkg, float scale, int load_c2);

struct dtex_rrp* dtex_load_rrp(struct dtex_import_stream* is);

struct dtex_pts* dtex_load_pts(struct dtex_import_stream* is);

struct dtex_rrr* dtex_load_rrr(struct dtex_import_stream* is);

struct dtex_b4r* dtex_load_b4r(struct dtex_import_stream* is);

#endif // dynamic_texture_desc_loader_h

#ifdef __cplusplus
}
#endif