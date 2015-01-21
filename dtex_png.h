#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_png_h
#define dynamic_texture_png_h

#include <stdint.h>

#define USE_LIBPNG

// todo cache buf for source
uint8_t* dtex_png_read(const char* filepath, int* width, int* height, int* channels, int* format);

#endif // dynamic_texture_png_h

#ifdef __cplusplus
}
#endif