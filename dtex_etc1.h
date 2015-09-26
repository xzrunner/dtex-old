#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#ifndef dynamic_texture_etc1_h
#define dynamic_texture_etc1_h

#include <opengl.h>

uint8_t* dtex_etc1_decode(const uint8_t* buf, int width, int height);
uint8_t* dtex_etc1_encode(const uint8_t* buf, int width, int height);

uint8_t* dtex_etc1_read_file(const char* filepath, uint32_t* width, uint32_t* height);

void dtex_etc1_gen_texture(uint8_t* data, int width, int height, GLuint* id_rgb, GLuint* id_alpha);

#endif // dynamic_texture_etc1_h

#ifdef __cplusplus
}
#endif