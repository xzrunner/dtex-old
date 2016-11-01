#include "dtex_png.h"
#include "dtex_typedef.h"

#include <fs_file.h>
#include <fault.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <png.h>

static int _offset = 0;
static inline void
_read_memory_cb(png_structp png, png_bytep data, png_size_t size) {
	char* raw = (char*) png_get_io_ptr(png);
	memcpy(data, raw + _offset, size);
	_offset += size;
}

uint8_t* 
dtex_png_read(const char* filepath, int* width, int* height, int* channels, int* format) {
	struct fs_file* file = fs_open(filepath, "rb");
	if (file == NULL) {
//		fault("Can't open png file: %s\n", filepath);
		return NULL;
	}
	
	size_t sz = fs_size(file);
	uint8_t* buf = (uint8_t*)malloc(sz);
	if (fs_read(file, buf, sz) != sz) {
//		fault("Invalid uncompress data source\n");
		fs_close(file);
		free(buf);		
		return NULL;
	}
	fs_close(file);

	_offset = 0;

	png_byte lHeader[8];
	png_structp lPngPtr = NULL; png_infop lInfoPtr = NULL;
	png_byte* lImageBuffer = NULL; png_bytep* lRowPtrs = NULL;
	png_int_32 lRowSize; bool lTransparency;
	do
	{
		// 		if (m_resource.read(lHeader, sizeof(lHeader)) == 0)
		// 			break;
		memcpy(lHeader, (char*)buf + _offset, sizeof(lHeader));
		_offset += sizeof(lHeader);
		if (png_sig_cmp(lHeader, 0, 8) != 0) break;

		lPngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		if (!lPngPtr) break;
		lInfoPtr = png_create_info_struct(lPngPtr);
		if (!lInfoPtr) break;
		png_set_read_fn(lPngPtr, buf, _read_memory_cb);
		if (setjmp(png_jmpbuf(lPngPtr))) break;

		png_set_sig_bytes(lPngPtr, 8);
		png_read_info(lPngPtr, lInfoPtr);
		png_int_32 lDepth, lColorType;
		png_uint_32 lWidth, lHeight;
		png_get_IHDR(lPngPtr, lInfoPtr, &lWidth, &lHeight, &lDepth, &lColorType, NULL, NULL, NULL);
		*width = lWidth; *height = lHeight;

		// Creates a full alpha channel if transparency is encoded as
		// an array of palette entries or a single transparent color.
		lTransparency = false;
		if (png_get_valid(lPngPtr, lInfoPtr, PNG_INFO_tRNS)) 
		{
			png_set_tRNS_to_alpha(lPngPtr);
			lTransparency = true;
			break;
		}
		// Expands PNG with less than 8bits per channel to 8bits.
		if (lDepth < 8) 
		{
			png_set_packing (lPngPtr);
		} 
		// Shrinks PNG with 16bits per color channel down to 8bits.
		else if (lDepth == 16) 
		{
			png_set_strip_16(lPngPtr);
		}
		// Indicates that image needs conversion to RGBA if needed.
		switch (lColorType) 
		{
		case PNG_COLOR_TYPE_PALETTE:
			png_set_palette_to_rgb(lPngPtr);
			*format = lTransparency ? PIXEL_RGBA : PIXEL_RGB;
			*channels = lTransparency ? 4 : 3;
			break;
		case PNG_COLOR_TYPE_RGB:
			png_set_add_alpha(lPngPtr, 0xff, PNG_FILLER_AFTER);
			*format = PIXEL_RGBA;
			*channels = 4;
			break;
		case PNG_COLOR_TYPE_RGBA:
			*format = PIXEL_RGBA;
			*channels = 4;
			break;
		case PNG_COLOR_TYPE_GRAY:
			png_set_expand_gray_1_2_4_to_8(lPngPtr);
			*format = lTransparency ? PIXEL_LUMINANCE_ALPHA : PIXEL_LUMINANCE;
			*channels = 1;
			break;
		case PNG_COLOR_TYPE_GA:
			png_set_expand_gray_1_2_4_to_8(lPngPtr);
			*format = PIXEL_LUMINANCE_ALPHA;
			*channels = 1;
			break;
		}
		png_read_update_info(lPngPtr, lInfoPtr);

		lRowSize = png_get_rowbytes(lPngPtr, lInfoPtr);
		if (lRowSize <= 0) break;
		lImageBuffer = (png_byte*)malloc(lRowSize * lHeight * sizeof(png_byte));
		if (!lImageBuffer) break;
		lRowPtrs = (png_bytep*)malloc(lHeight * sizeof(png_bytep));
		if (!lRowPtrs) break;
		for (unsigned int i = 0; i < lHeight; ++i) 
		{
			lRowPtrs[lHeight - (i + 1)] = lImageBuffer + i * lRowSize;
		}
		png_read_image(lPngPtr, lRowPtrs);

		png_destroy_read_struct(&lPngPtr, &lInfoPtr, NULL);
		free(lRowPtrs);

		free(buf);

		return lImageBuffer;
	} while (0);

	// error
	//ERROR:
	//		Log::error("Error while reading PNG file");
	free(lRowPtrs); free(lImageBuffer);
	free(buf);
	if (lPngPtr != NULL) 
	{
		png_infop* lInfoPtrP = lInfoPtr != NULL ? &lInfoPtr : NULL;
		png_destroy_read_struct(&lPngPtr, lInfoPtrP, NULL);
	}
	return NULL;

	free(buf);
}
