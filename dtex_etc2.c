#include "dtex_etc2.h"
#include "dtex_math.h"
#include "dtex_gl.h"
#include "dtex_statistics.h"

#include <c_wrap_etcpack.h>

#include <fault.h>
#include <fs_file.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static inline void 
read_color_block_etc(uint8_t** stream, unsigned int* block1, unsigned int* block2) {
	uint8_t *data = *stream;
	*block1 = 0;			*block1 |= data[0];
	*block1 = *block1 << 8; *block1 |= data[1];
	*block1 = *block1 << 8; *block1 |= data[2];
	*block1 = *block1 << 8; *block1 |= data[3];
	*block2 = 0;			*block2 |= data[4];
	*block2 = *block2 << 8; *block2 |= data[5];
	*block2 = *block2 << 8; *block2 |= data[6];
	*block2 = *block2 << 8; *block2 |= data[7];
	*stream = data + 8;
}

uint8_t* 
dtex_etc2_decode(const uint8_t* buf, int width, int height, int type) {
	etcpack_init();

	assert(IS_POT(width) && IS_POT(height));

	int bpp = 4;
	uint8_t* dst = (uint8_t*)malloc(width * height * bpp);
	if (dst == NULL) {
		fault("Fail to malloc (dtex_etc2_decode)");
	}
	memset(dst, 0x00, width * height * bpp);

	uint8_t rgba[4*4*4], *lb;
	unsigned int block1, block2;
	int lx, ly, lw, lh;

	uint8_t default_alpha = 255;

	uint8_t* ptr_src = (uint8_t*)buf;
	for (int y = 0; y < height; y += 4) {
		for (int x = 0; x < width; x += 4) {
			switch (type)
			{
			case ETC2PACKAGE_RGB_NO_MIPMAPS:
				read_color_block_etc(&ptr_src, &block1, &block2);
				etcpack_unpack_etc2c(block1, block2, rgba, 4, 4, 0, 0, 4);
				lb = rgba;
				lh = MIN(y + 4, height) - y;
				lw = MIN(x + 4, width) - x;
				for (ly = 0; ly < lh; ly++,lb+=4*4)
					for(lx = 0; lx < lw; lx++) {
						memcpy(dst + (width*(y + ly) + x + lx)*bpp, lb + lx*4, 3);
						memcpy(dst + (width*(y + ly) + x + lx)*bpp + 3, &default_alpha, 1);
					}
				break;
			case ETC2PACKAGE_RGBA_NO_MIPMAPS:
				etcpack_unpack_alpha_c(ptr_src, rgba+3, 4, 4, 0, 0, 4);
				ptr_src += 8;
				read_color_block_etc(&ptr_src, &block1, &block2);

				etcpack_unpack_etc2c(block1, block2, rgba, 4, 4, 0, 0, 4);
				lb = rgba;
				lh = MIN(y + 4, height) - y;
				lw = MIN(x + 4, width) - x;
				for (ly = 0; ly < lh; ++ly, lb += 4*4)
					for(lx = 0; lx < lw; ++lx)
						memcpy(dst + (width * (y + ly) + x + lx) * bpp, lb + lx * 4, 4);
				break;
			case ETC2PACKAGE_RGBA1_NO_MIPMAPS:
				read_color_block_etc(&ptr_src, &block1, &block2);
				etcpack_unpack_etc21bitalphac(block1, block2, rgba, NULL, 4, 4, 0, 0, 4);
				lb = rgba;
				lh = MIN(y + 4, height) - y;
				lw = MIN(x + 4, width) - x;
				for (ly = 0; ly < lh; ly++,lb+=4*4)
					for(lx = 0; lx < lw; lx++)
						memcpy(dst + (width*(y + ly) + x + lx)*bpp, lb + lx*4, 4);
				break;
			}
		}
	}

	return dst;
}

uint8_t* 
dtex_etc2_encode(const uint8_t* buf, int width, int height) {
	return NULL;
}

struct PKMHeader {
	unsigned char magic[4];
	unsigned char version[2];
	unsigned char typeMSB;
	unsigned char typeLSB;
	unsigned char paddedWidthMSB;
	unsigned char paddedWidthLSB;
	unsigned char paddedHeightMSB;
	unsigned char paddedHeightLSB;
	unsigned char widthMSB;
	unsigned char widthLSB;
	unsigned char heightMSB;
	unsigned char heightLSB;
};

uint8_t* 
dtex_etc2_read_file(const char* filepath, uint32_t* width, uint32_t* height, int* type) {
	struct fs_file* file = fs_open(filepath, "rb");
	if (file == NULL) {
		assert(0);
		fault("Can't open etc2 file: %s\n", filepath);
	}
	
	struct PKMHeader header;
	fs_read(file, &header, sizeof(header));
	*type = (header.typeMSB << 8) | header.typeLSB;

	*width = (header.paddedWidthMSB << 8) | header.paddedWidthLSB,
	*height = (header.paddedHeightMSB << 8) | header.paddedHeightLSB;
	size_t sz;
	if (*type == ETC2PACKAGE_RGBA_NO_MIPMAPS) {
		sz = *width * *height;
	} else {
		sz = *width * *height / 2;
	}
	uint8_t* buf = (uint8_t*)malloc(sz);
	if (buf == NULL) {
		fault("Fail to malloc (dtex_etc2_read_file)");
	}
	if (fs_read(file, buf, sz) != 1) {
		fault("Invalid uncompress data source\n");
	}
	fs_close(file);	

	return buf;
}

uint8_t* 
dtex_etc2_init_blank(int edge) {
	assert(IS_POT(edge));

	size_t sz = edge * edge;
	uint8_t* buf = (uint8_t*)malloc(sz);
	memset(buf, 0, sz);
	
	return buf;
}