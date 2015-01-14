#ifndef dynamic_texture_pvr_h
#define dynamic_texture_pvr_h

#include <stdint.h>

// only support rgba 4bpp now
void dtex_pvr_decode(uint8_t* dst, const uint8_t* src, int width, int height);
void dtex_pvr_encode(uint8_t* dst, const uint8_t* src, int width, int height);

#endif // dynamic_texture_pvr_h