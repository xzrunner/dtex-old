#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#ifndef dynamic_texture_etc1_h
#define dynamic_texture_etc1_h

#ifdef USED_IN_EDITOR

uint8_t* dtex_etc1_decode(const uint8_t* buf, int width, int height);
uint8_t* dtex_etc1_encode(const uint8_t* buf, int width, int height);

uint8_t* dtex_etc1_read_file(const char* filepath, uint32_t* width, uint32_t* height);

#endif // USED_IN_EDITOR

#endif // dynamic_texture_etc1_h

#ifdef __cplusplus
}
#endif