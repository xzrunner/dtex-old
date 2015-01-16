#ifndef dynamic_texture_png_h
#define dynamic_texture_png_h

#include <stdint.h>

// todo cache buf for source
uint8_t* dtex_png_read(const char* filepath, int* width, int* height, int* channels, int* format);

#endif // dynamic_texture_png_h