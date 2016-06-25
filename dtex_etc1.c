#include "dtex_etc1.h"
#include "dtex_math.h"
#include "dtex_gl.h"
#include "dtex_statistics.h"

#include <fault.h>
#include <fs_file.h>
#include <rg_etc1_for_c.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

uint8_t* 
dtex_etc1_decode(const uint8_t* buf, int width, int height) {
	assert(IS_POT(width) && IS_POT(height));

	uint8_t* dst = (uint8_t*)malloc(width * height * 4);
	if (dst == NULL) {
		fault("Fail to malloc (dtex_etc1_decode)");
	}
	memset(dst, 0x00, width * height * 4);
	const uint8_t* ptr_src = buf;

	int bw = width / 4;
	int bh = height / 4;
	for (int y = 0; y < bh; ++y) {
		for (int x = 0; x < bw; ++x) {
			uint32_t block[16];
			rg_etc1_unpack_block(ptr_src, block, false);
			ptr_src += 8;
			for (int iy = 0; iy < 4; ++iy) {
				memcpy(dst + ((y * 4 + iy) * width + x * 4) * 4, block + 4 * iy, 16);
			}
		}
	}	

	return dst;
}

uint8_t* 
dtex_etc1_encode(const uint8_t* buf, int width, int height) {
	assert(IS_POT(width) && IS_POT(height));

	rg_etc1_pack_block_init();

	int bw = width / 4;
	int bh = height / 4;

	size_t sz = bw * bh * 8;
	uint8_t* dst = (uint8_t*)malloc(sz);

	for (int y = 0; y < bh; ++y) {
		for (int x = 0; x < bw; ++x) {
			uint32_t block[16];
			for (int iy = 0; iy < 4; iy++) {
				memcpy(block + 4 * iy, buf + ((y * 4 + iy) * width + x * 4) * 4, 16);
			}
			rg_etc1_pack_block(dst + (bw * y + x) * 8, block, cHighQuality, false);
		}
	}

	return dst;
}

struct PKMHeader {
	unsigned char identifier[8];
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
dtex_etc1_read_file(const char* filepath, uint32_t* width, uint32_t* height) {
	struct fs_file* file = fs_open(filepath, "rb");
	if (file == NULL) {
		assert(0);
		fault("Can't open etc1 file: %s\n", filepath);
	}
	
	struct PKMHeader header;
	fs_read(file, &header, sizeof(header));

	*width = (header.paddedWidthMSB << 8) | header.paddedWidthLSB,
	*height = (header.paddedHeightMSB << 8) | header.paddedHeightLSB;
	size_t sz = *width * *height / 2;
	uint8_t* buf = (uint8_t*)malloc(sz);
	if (buf == NULL) {
		fault("Fail to malloc (dtex_etc1_read_file)");
	}
	if (fs_read(file, buf, sz) != 1) {
		fault("Invalid uncompress data source\n");
	}
	fs_close(file);	

	return buf;
}
