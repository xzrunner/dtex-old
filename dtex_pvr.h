#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_pvr_h
#define dynamic_texture_pvr_h

#include <stdint.h>

typedef unsigned int GLuint;

#define COMPRESSED_RGBA_PVRTC_4BPPV1_IMG 4
#define COMPRESSED_RGBA_PVRTC_2BPPV1_IMG 2

// only support rgba 4bpp now
uint8_t* dtex_pvr_decode(const uint8_t* buf, int width, int height);
uint8_t* dtex_pvr_encode(const uint8_t* buf, int width, int height);

uint8_t* dtex_pvr_read_file(const char* filepath, uint32_t* width, uint32_t* height);
void dtex_pvr_write_file(const char* filepath, const uint8_t* buf, uint32_t width, uint32_t height);

unsigned dtex_pvr_get_morton_number(int x, int y);

uint8_t* dtex_pvr_init_blank(int edge);

GLuint dtex_pvr_gen_texture(uint8_t* data, int internal_format, int width, int height);

#endif // dynamic_texture_pvr_h

#ifdef __cplusplus
}
#endif