#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_bitmap_h
#define dynamic_texture_bitmap_h

#include <stdint.h>

uint8_t* dtex_bmp_init_blank(int edge);

void dtex_bmp_revert_y(uint32_t* data, int width, int height);

#endif // dynamic_texture_bitmap_h

#ifdef __cplusplus
}
#endif