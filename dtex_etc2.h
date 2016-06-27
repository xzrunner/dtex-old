#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#ifndef dynamic_texture_etc2_h
#define dynamic_texture_etc2_h

enum DTEX_ETC_TYPE
{
	ETC1_RGB_NO_MIPMAPS = 0,
	ETC2PACKAGE_RGB_NO_MIPMAPS,
	ETC2PACKAGE_RGBA_NO_MIPMAPS_OLD,
	ETC2PACKAGE_RGBA_NO_MIPMAPS,
	ETC2PACKAGE_RGBA1_NO_MIPMAPS,
	ETC2PACKAGE_R_NO_MIPMAPS,
	ETC2PACKAGE_RG_NO_MIPMAPS,
	ETC2PACKAGE_R_SIGNED_NO_MIPMAPS,
	ETC2PACKAGE_RG_SIGNED_NO_MIPMAPS
};

uint8_t* dtex_etc2_decode(const uint8_t* buf, int width, int height, int type);
uint8_t* dtex_etc2_encode(const uint8_t* buf, int width, int height);

uint8_t* dtex_etc2_read_file(const char* filepath, uint32_t* width, uint32_t* height, int* type);

uint8_t* dtex_etc2_init_blank(int edge);

#endif // dynamic_texture_etc2_h

#ifdef __cplusplus
}
#endif