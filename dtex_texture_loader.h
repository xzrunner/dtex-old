#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_texture_loader_h
#define dynamic_texture_texture_loader_h

#include <stdint.h>

void dtex_load_png(uint8_t* buf, int format, struct dtex_raw_tex* tex);

void dtex_load_pvr(uint8_t* buf, size_t sz, struct dtex_raw_tex* tex);

void dtex_load_etc1(uint8_t* buf, struct dtex_raw_tex* tex);

#endif // dynamic_texture_texture_loader_h

#ifdef __cplusplus
}
#endif