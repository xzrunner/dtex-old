#include "dtex_pvr.h"

#include <stdbool.h>
#include <string.h>
#include <assert.h>

struct pvrtc_packet {
	unsigned int    modulationData;
	unsigned        usePunchthroughAlpha : 1;
	unsigned        colorA          	 : 14;
	unsigned        colorAIsOpaque  	 : 1;
	unsigned        colorB        		 : 15;
	unsigned        colorBIsOpaque  	 : 1;
};

struct color_rgba_char {
	uint8_t r, g, b, a;
};

struct color_rgba_int {
	int r, g, b, a;
};

static const uint8_t 
BITSCALE_5_TO_8[32] = {
 0, 8, 16, 24, 32, 41, 49, 57, 65, 74,
 82, 90, 98, 106, 115, 123, 131, 139, 148, 156,
 164, 172, 180, 189, 197, 205, 213, 222, 230, 238,
 246, 255
};

static const uint8_t 
BITSCALE_4_TO_8[16] = {
 0, 17, 34, 51, 68, 85, 102, 119, 136, 153,
 170, 187, 204, 221, 238, 255
};

static const uint8_t 
BITSCALE_3_TO_8[8] = {
 0, 36, 72, 109, 145, 182, 218, 255
};

static const uint8_t 
BITSCALE_8_TO_5_FLOOR[256] = {
 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
 1, 1, 1, 1, 1, 1, 1, 2, 2, 2,
 2, 2, 2, 2, 2, 3, 3, 3, 3, 3,
 3, 3, 3, 4, 4, 4, 4, 4, 4, 4,
 4, 4, 5, 5, 5, 5, 5, 5, 5, 5,
 6, 6, 6, 6, 6, 6, 6, 6, 7, 7,
 7, 7, 7, 7, 7, 7, 8, 8, 8, 8,
 8, 8, 8, 8, 8, 9, 9, 9, 9, 9,
 9, 9, 9, 10, 10, 10, 10, 10, 10, 10,
 10, 11, 11, 11, 11, 11, 11, 11, 11, 12,
 12, 12, 12, 12, 12, 12, 12, 13, 13, 13,
 13, 13, 13, 13, 13, 13, 14, 14, 14, 14,
 14, 14, 14, 14, 15, 15, 15, 15, 15, 15,
 15, 15, 16, 16, 16, 16, 16, 16, 16, 16,
 17, 17, 17, 17, 17, 17, 17, 17, 17, 18,
 18, 18, 18, 18, 18, 18, 18, 19, 19, 19,
 19, 19, 19, 19, 19, 20, 20, 20, 20, 20,
 20, 20, 20, 21, 21, 21, 21, 21, 21, 21,
 21, 22, 22, 22, 22, 22, 22, 22, 22, 22,
 23, 23, 23, 23, 23, 23, 23, 23, 24, 24,
 24, 24, 24, 24, 24, 24, 25, 25, 25, 25,
 25, 25, 25, 25, 26, 26, 26, 26, 26, 26,
 26, 26, 26, 27, 27, 27, 27, 27, 27, 27,
 27, 28, 28, 28, 28, 28, 28, 28, 28, 29,
 29, 29, 29, 29, 29, 29, 29, 30, 30, 30,
 30, 30, 30, 30, 30, 31
};

static const uint8_t 
BITSCALE_8_TO_4_FLOOR[256] = {
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
 1, 1, 1, 1, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 2, 3, 3, 3, 3, 3, 3, 3, 3, 3,
 3, 3, 3, 3, 3, 3, 3, 3, 4, 4,
 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
 4, 4, 4, 4, 4, 5, 5, 5, 5, 5,
 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
 5, 5, 6, 6, 6, 6, 6, 6, 6, 6,
 6, 6, 6, 6, 6, 6, 6, 6, 6, 7,
 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
 7, 7, 7, 7, 7, 7, 8, 8, 8, 8,
 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
 8, 8, 8, 9, 9, 9, 9, 9, 9, 9,
 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
 10, 10, 10, 10, 10, 10, 10, 11, 11, 11,
 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
 11, 11, 11, 11, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
 12, 13, 13, 13, 13, 13, 13, 13, 13, 13,
 13, 13, 13, 13, 13, 13, 13, 13, 14, 14,
 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
 14, 14, 14, 14, 14, 15
};

static const uint8_t 
BITSCALE_8_TO_3_FLOOR[256] = {
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
 1, 1, 1, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
 3, 3, 3, 3, 3, 3, 4, 4, 4, 4,
 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
 4, 4, 4, 5, 5, 5, 5, 5, 5, 5,
 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
 5, 5, 5, 5, 5, 5, 5, 5, 5, 6,
 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
 6, 6, 6, 6, 6, 7
};

static const uint8_t 
BITSCALE_8_TO_5_CEIL[256] = {
 0, 1, 1, 1, 1, 1, 1, 1, 1, 2,
 2, 2, 2, 2, 2, 2, 2, 3, 3, 3,
 3, 3, 3, 3, 3, 4, 4, 4, 4, 4,
 4, 4, 4, 5, 5, 5, 5, 5, 5, 5,
 5, 5, 6, 6, 6, 6, 6, 6, 6, 6,
 7, 7, 7, 7, 7, 7, 7, 7, 8, 8,
 8, 8, 8, 8, 8, 8, 9, 9, 9, 9,
 9, 9, 9, 9, 9, 10, 10, 10, 10, 10,
 10, 10, 10, 11, 11, 11, 11, 11, 11, 11,
 11, 12, 12, 12, 12, 12, 12, 12, 12, 13,
 13, 13, 13, 13, 13, 13, 13, 14, 14, 14,
 14, 14, 14, 14, 14, 14, 15, 15, 15, 15,
 15, 15, 15, 15, 16, 16, 16, 16, 16, 16,
 16, 16, 17, 17, 17, 17, 17, 17, 17, 17,
 18, 18, 18, 18, 18, 18, 18, 18, 18, 19,
 19, 19, 19, 19, 19, 19, 19, 20, 20, 20,
 20, 20, 20, 20, 20, 21, 21, 21, 21, 21,
 21, 21, 21, 22, 22, 22, 22, 22, 22, 22,
 22, 23, 23, 23, 23, 23, 23, 23, 23, 23,
 24, 24, 24, 24, 24, 24, 24, 24, 25, 25,
 25, 25, 25, 25, 25, 25, 26, 26, 26, 26,
 26, 26, 26, 26, 27, 27, 27, 27, 27, 27,
 27, 27, 27, 28, 28, 28, 28, 28, 28, 28,
 28, 29, 29, 29, 29, 29, 29, 29, 29, 30,
 30, 30, 30, 30, 30, 30, 30, 31, 31, 31,
 31, 31, 31, 31, 31, 31
};

static const uint8_t 
BITSCALE_8_TO_4_CEIL[256] = {
 0, 1, 1, 1, 1, 1, 1, 1, 1, 1,
 1, 1, 1, 1, 1, 1, 1, 1, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 3, 3, 3, 3, 3,
 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
 4, 4, 4, 4, 4, 4, 4, 4, 4, 5,
 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
 5, 5, 5, 5, 5, 5, 6, 6, 6, 6,
 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
 6, 6, 6, 7, 7, 7, 7, 7, 7, 7,
 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
 8, 8, 8, 8, 8, 8, 8, 9, 9, 9,
 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
 9, 9, 9, 9, 10, 10, 10, 10, 10, 10,
 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
 10, 11, 11, 11, 11, 11, 11, 11, 11, 11,
 11, 11, 11, 11, 11, 11, 11, 11, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 13, 13, 13, 13, 13,
 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
 13, 13, 14, 14, 14, 14, 14, 14, 14, 14,
 14, 14, 14, 14, 14, 14, 14, 14, 14, 15,
 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
 15, 15, 15, 15, 15, 15
};

static const uint8_t 
BITSCALE_8_TO_3_CEIL[256] = {
 0, 1, 1, 1, 1, 1, 1, 1, 1, 1,
 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
 1, 1, 1, 1, 1, 1, 1, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 3, 3, 3, 3, 3, 3, 3,
 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
 4, 4, 4, 4, 4, 4, 5, 5, 5, 5,
 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
 5, 5, 5, 6, 6, 6, 6, 6, 6, 6,
 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
 6, 6, 6, 6, 6, 6, 6, 6, 6, 7,
 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
 7, 7, 7, 7, 7, 7
};

static inline void
GetColorRgbaA(const struct pvrtc_packet* packet, struct color_rgba_int* col) {
	if (packet->colorAIsOpaque) {
		unsigned char r = packet->colorA >> 9;
		unsigned char g = packet->colorA >> 4 & 0x1f;
		unsigned char b = packet->colorA & 0xf;
		col->r = BITSCALE_5_TO_8[r];
		col->g = BITSCALE_5_TO_8[g];
		col->b = BITSCALE_4_TO_8[b];
		col->a = 255;
	} else {
		unsigned char a = packet->colorA >> 11 & 7;
		unsigned char r = packet->colorA >> 7 & 0xf;
		unsigned char g = packet->colorA >> 3 & 0xf;
		unsigned char b = packet->colorA & 7;		
		col->r = BITSCALE_4_TO_8[r];
		col->g = BITSCALE_4_TO_8[g];
		col->b = BITSCALE_3_TO_8[b];
		col->a = BITSCALE_3_TO_8[a];
	}
}

static inline void
GetColorRgbaB(const struct pvrtc_packet* packet, struct color_rgba_int* col) {
	if(packet->colorBIsOpaque) {
		unsigned char r = packet->colorB >> 10;
		unsigned char g = packet->colorB >> 5 & 0x1f;
		unsigned char b = packet->colorB & 0x1f;
		col->r = BITSCALE_5_TO_8[r];
		col->g = BITSCALE_5_TO_8[g];
		col->b = BITSCALE_5_TO_8[b];
		col->a = 255;
	} else {
		unsigned char a = packet->colorB >> 12 & 7;
		unsigned char r = packet->colorB >> 8 & 0xf;
		unsigned char g = packet->colorB >> 4 & 0xf;
		unsigned char b = packet->colorB & 0xf;
		col->r = BITSCALE_4_TO_8[r];
		col->g = BITSCALE_4_TO_8[g];
		col->b = BITSCALE_4_TO_8[b];
		col->a = BITSCALE_3_TO_8[a];
	}	
}

static inline void
SetColorA(struct pvrtc_packet* packet, struct color_rgba_char* col) {
	int a = BITSCALE_8_TO_3_FLOOR[col->a];
	if(a == 7) {
		int r = BITSCALE_8_TO_5_FLOOR[col->r];
		int g = BITSCALE_8_TO_5_FLOOR[col->g];
		int b = BITSCALE_8_TO_4_FLOOR[col->b];
		packet->colorA = r<<9 | g<<4 | b;
		packet->colorAIsOpaque = true;
	} else {
		int r = BITSCALE_8_TO_4_FLOOR[col->r];
		int g = BITSCALE_8_TO_4_FLOOR[col->g];
		int b = BITSCALE_8_TO_3_FLOOR[col->b];
		packet->colorA = a<<11 | r<<7 | g<<3 | b;
		packet->colorAIsOpaque = false;
	}	
}

static inline void
SetColorB(struct pvrtc_packet* packet, struct color_rgba_char* col) {
	int a = BITSCALE_8_TO_3_CEIL[col->a];
	if(a == 7) {
		int r = BITSCALE_8_TO_5_CEIL[col->r];
		int g = BITSCALE_8_TO_5_CEIL[col->g];
		int b = BITSCALE_8_TO_5_CEIL[col->b];
		packet->colorB = r<<10 | g<<5 | b;
		packet->colorBIsOpaque = true;
	} else {
		int r = BITSCALE_8_TO_4_CEIL[col->r];
		int g = BITSCALE_8_TO_4_CEIL[col->g];
		int b = BITSCALE_8_TO_4_CEIL[col->b];
		packet->colorB = a<<12 | r<<8 | g<<4 | b;
		packet->colorBIsOpaque = false;
	}
}

static const unsigned char 
BILINEAR_FACTORS[16][4] = {
	{ 4, 4, 4, 4 },
	{ 2, 6, 2, 6 },
	{ 8, 0, 8, 0 },
	{ 6, 2, 6, 2 },
	
	{ 2, 2, 6, 6 },
	{ 1, 3, 3, 9 },
	{ 4, 0, 12, 0 },
	{ 3, 1, 9, 3 },
	
	{ 8, 8, 0, 0 },
	{ 4, 12, 0, 0 },
	{ 16, 0, 0, 0 },
	{ 12, 4, 0, 0 },
	
	{ 6, 6, 2, 2 },
	{ 3, 9, 1, 3 },
	{ 12, 0, 4, 0 },
	{ 9, 3, 3, 1 },
};

// Weights are { colorA, colorB, alphaA, alphaB }
static const unsigned char 
WEIGHTS[8][4] = {
	// Weights for Mode=0
	{ 8, 0, 8, 0 },
	{ 5, 3, 5, 3 },
	{ 3, 5, 3, 5 },
	{ 0, 8, 0, 8 },
	
	// Weights for Mode=1
	{ 8, 0, 8, 0 },
	{ 4, 4, 4, 4 },
	{ 4, 4, 0, 0 },
	{ 0, 8, 0, 8 },
};

static const unsigned short 
MORTON_TABLE[256] = {
    0x0000, 0x0001, 0x0004, 0x0005, 0x0010, 0x0011, 0x0014, 0x0015,
    0x0040, 0x0041, 0x0044, 0x0045, 0x0050, 0x0051, 0x0054, 0x0055,
    0x0100, 0x0101, 0x0104, 0x0105, 0x0110, 0x0111, 0x0114, 0x0115,
    0x0140, 0x0141, 0x0144, 0x0145, 0x0150, 0x0151, 0x0154, 0x0155,
    0x0400, 0x0401, 0x0404, 0x0405, 0x0410, 0x0411, 0x0414, 0x0415,
    0x0440, 0x0441, 0x0444, 0x0445, 0x0450, 0x0451, 0x0454, 0x0455,
    0x0500, 0x0501, 0x0504, 0x0505, 0x0510, 0x0511, 0x0514, 0x0515,
    0x0540, 0x0541, 0x0544, 0x0545, 0x0550, 0x0551, 0x0554, 0x0555,
    0x1000, 0x1001, 0x1004, 0x1005, 0x1010, 0x1011, 0x1014, 0x1015,
    0x1040, 0x1041, 0x1044, 0x1045, 0x1050, 0x1051, 0x1054, 0x1055,
    0x1100, 0x1101, 0x1104, 0x1105, 0x1110, 0x1111, 0x1114, 0x1115,
    0x1140, 0x1141, 0x1144, 0x1145, 0x1150, 0x1151, 0x1154, 0x1155,
    0x1400, 0x1401, 0x1404, 0x1405, 0x1410, 0x1411, 0x1414, 0x1415,
    0x1440, 0x1441, 0x1444, 0x1445, 0x1450, 0x1451, 0x1454, 0x1455,
    0x1500, 0x1501, 0x1504, 0x1505, 0x1510, 0x1511, 0x1514, 0x1515,
    0x1540, 0x1541, 0x1544, 0x1545, 0x1550, 0x1551, 0x1554, 0x1555,
    0x4000, 0x4001, 0x4004, 0x4005, 0x4010, 0x4011, 0x4014, 0x4015,
    0x4040, 0x4041, 0x4044, 0x4045, 0x4050, 0x4051, 0x4054, 0x4055,
    0x4100, 0x4101, 0x4104, 0x4105, 0x4110, 0x4111, 0x4114, 0x4115,
    0x4140, 0x4141, 0x4144, 0x4145, 0x4150, 0x4151, 0x4154, 0x4155,
    0x4400, 0x4401, 0x4404, 0x4405, 0x4410, 0x4411, 0x4414, 0x4415,
    0x4440, 0x4441, 0x4444, 0x4445, 0x4450, 0x4451, 0x4454, 0x4455,
    0x4500, 0x4501, 0x4504, 0x4505, 0x4510, 0x4511, 0x4514, 0x4515,
    0x4540, 0x4541, 0x4544, 0x4545, 0x4550, 0x4551, 0x4554, 0x4555,
    0x5000, 0x5001, 0x5004, 0x5005, 0x5010, 0x5011, 0x5014, 0x5015,
    0x5040, 0x5041, 0x5044, 0x5045, 0x5050, 0x5051, 0x5054, 0x5055,
    0x5100, 0x5101, 0x5104, 0x5105, 0x5110, 0x5111, 0x5114, 0x5115,
    0x5140, 0x5141, 0x5144, 0x5145, 0x5150, 0x5151, 0x5154, 0x5155,
    0x5400, 0x5401, 0x5404, 0x5405, 0x5410, 0x5411, 0x5414, 0x5415,
    0x5440, 0x5441, 0x5444, 0x5445, 0x5450, 0x5451, 0x5454, 0x5455,
    0x5500, 0x5501, 0x5504, 0x5505, 0x5510, 0x5511, 0x5514, 0x5515,
    0x5540, 0x5541, 0x5544, 0x5545, 0x5550, 0x5551, 0x5554, 0x5555
};

static inline unsigned 
GetMortonNumber(int x, int y) {
    return MORTON_TABLE[x >> 8] << 17 | MORTON_TABLE[y >> 8] << 16 | MORTON_TABLE[x & 0xFF] << 1 | MORTON_TABLE[y & 0xFF];
}

static inline void
_cal_color_ab(const struct pvrtc_packet* packets, const unsigned char (*factor)[4], 
	int x0, int x1, int y0, int y1, struct color_rgba_int* ca, struct color_rgba_int* cb) {
	const struct pvrtc_packet* p[4];
	p[0] = packets + GetMortonNumber(x0, y0);
	p[1] = packets + GetMortonNumber(x1, y0);
	p[2] = packets + GetMortonNumber(x0, y1);
	p[3] = packets + GetMortonNumber(x1, y1);

	memset(ca, 0, sizeof(*ca));
	for (int i = 0; i < 4; ++i) {
		struct color_rgba_int c;
		GetColorRgbaA(p[i], &c);
		uint8_t f = (*factor)[i];
		ca->r += c.r * f;
		ca->g += c.g * f;
		ca->b += c.b * f;
		ca->a += c.a * f;						
	}

	memset(cb, 0, sizeof(*cb));
	for (int i = 0; i < 4; ++i) {
		struct color_rgba_int c;
		GetColorRgbaB(p[i], &c);
		uint8_t f = (*factor)[i];
		cb->r += c.r * f;
		cb->g += c.g * f;
		cb->b += c.b * f;
		cb->a += c.a * f;
	}	
}

void 
dtex_pvr_decode(uint8_t* dst, const uint8_t* src, int width, int height) {
	assert(width == height);

	const int blocks = width >> 2;
	const int block_mask = blocks - 1;
	const struct pvrtc_packet* packets = (const struct pvrtc_packet*)src;

	for (int y = 0; y < blocks; ++y) {
		for (int x = 0; x < blocks; ++x) {
			const struct pvrtc_packet* packet = packets + GetMortonNumber(x, y);

			unsigned mod = packet->modulationData;
			const unsigned char (*weights)[4] = WEIGHTS + 4 * packet->usePunchthroughAlpha;
			const unsigned char (*factor)[4] = BILINEAR_FACTORS;

			for (int py = 0; py < 4; ++py) {
				const int yoffset = (py < 2) ? -1 : 0;
				const int y0 = (y + yoffset) & block_mask;
				const int y1 = (y0+1) & block_mask;

				for (int px = 0; px < 4; ++px) {
					const int xoffset = (px < 2) ? -1 : 0;
					const int x0 = (x + xoffset) & block_mask;
					const int x1 = (x0+1) & block_mask;

					struct color_rgba_int ca, cb;
					_cal_color_ab(packets, factor, x0, x1, y0, y1, &ca, &cb);
					
					const unsigned char* w = weights[mod & 3];
					struct color_rgba_char c;
					c.r = (ca.r * w[0] + cb.r * w[1]) >> 7;
					c.g = (ca.g * w[0] + cb.g * w[1]) >> 7;
					c.b = (ca.b * w[0] + cb.b * w[1]) >> 7;
					c.a = (ca.a * w[2] + cb.a * w[3]) >> 7;

					int ptr = (py+y*4) * width + (px+x*4);
					memcpy(&dst[ptr], &c, sizeof(c));

					mod >>= 2;
					factor++;					
				}
			}
		}
	}
}

#define isp2(x)	((x) > 0 && ((x) & ((x) - 1)) == 0)

struct col_bounding_box {
	struct color_rgba_char min, max;
};

static inline void
_cal_bounding_box(struct col_bounding_box* cbb, const uint8_t* src, int width, int xblock, int yblock) {
	int size = width;
	struct color_rgba_char* data = (struct color_rgba_char*)(src + yblock * 4 * size + xblock * 4);

	cbb->min = cbb->max = data[0];
	for (int i = 0; i < 4; ++i) {
		int ptr = size * i;
		for (int j = 0; j < 4; ++j) {
			struct color_rgba_char* c = &data[ptr+j];
			if (c->r < cbb->min.r) cbb->min.r = c->r;
			if (c->g < cbb->min.g) cbb->min.g = c->g;
			if (c->b < cbb->min.b) cbb->min.b = c->b;
			if (c->a < cbb->min.a) cbb->min.a = c->a;
			if (c->r > cbb->max.r) cbb->max.r = c->r;
			if (c->g > cbb->max.g) cbb->max.g = c->g;
			if (c->b > cbb->max.b) cbb->max.b = c->b;
			if (c->a > cbb->max.a) cbb->max.a = c->a;
		}
	}
}

static inline int
_color_mul(struct color_rgba_int* a, struct color_rgba_int* b) {
	return a->r * b->r + a->g * b->g + a->b * b->b + a->a * b->a;
}

static inline unsigned int
_rotate_right(unsigned int value, unsigned int shift) {
    if ((shift &= sizeof(value) * 8 - 1) == 0) {
        return value;
    }
    return (value >> shift) | (value << (sizeof(value) * 8 - shift));	
}

void 
dtex_pvr_encode(uint8_t* dst, const uint8_t* src, int width, int height) {
	assert(width == height);
	assert(isp2(width));

	const int size = width;
	const int blocks = size >> 2;
	const int block_mask = blocks - 1;

	struct pvrtc_packet* packets = (struct pvrtc_packet*)dst;

	for (int y = 0; y < blocks; ++y) {
		for (int x = 0; x < blocks; ++x) {
			struct col_bounding_box cbb;
			_cal_bounding_box(&cbb, src, width, x, y);
			struct pvrtc_packet* packet = packets + GetMortonNumber(x, y);
			packet->usePunchthroughAlpha = 0;
			SetColorA(packet, &cbb.min);
			SetColorB(packet, &cbb.max);
		}
	}

	for(int y = 0; y < blocks; ++y) {
		for(int x = 0; x < blocks; ++x) {
			const unsigned char (*factor)[4] = BILINEAR_FACTORS;
			const struct color_rgba_char* data = (const struct color_rgba_char*)(src + y * 4 * size + x * 4);

			uint32_t modulationData = 0;
			for(int py = 0; py < 4; ++py) {
				const int yoffset = (py < 2) ? -1 : 0;
				const int y0 = (y + yoffset) & block_mask;
				const int y1 = (y0 + 1) & block_mask;
				
				for(int px = 0; px < 4; ++px) {
					const int xoffset = (px < 2) ? -1 : 0;
					const int x0 = (x + xoffset) & block_mask;
					const int x1 = (x0 + 1) & block_mask;

					struct color_rgba_int ca, cb;
					_cal_color_ab(packets, factor, x0, x1, y0, y1, &ca, &cb);

					struct color_rgba_int d;
					d.r = cb.r - ca.r;
					d.g = cb.g - ca.g;
					d.b = cb.b - ca.b;
					d.a = cb.a - ca.a;

					struct color_rgba_int p;
					const struct color_rgba_char* pixel = (const struct color_rgba_char*)(data + py * size + px);
					p.r = pixel->r * 16;
					p.g = pixel->g * 16;
					p.b = pixel->b * 16;
					p.a = pixel->a * 16;

					struct color_rgba_int v;
					v.r = p.r - ca.r;
					v.g = p.g - ca.g;
					v.b = p.b - ca.b;
					v.a = p.a - ca.a;	

					// PVRTC uses weightings of 0, 3/8, 5/8 and 1
					// The boundaries for these are 3/16, 1/2 (=8/16), 13/16
					int projection = _color_mul(&v, &d) * 16;
					int len_squared = _color_mul(&d, &d);
					if (projection > 3 * len_squared) modulationData++;
					if (projection > 8 * len_squared) modulationData++;
					if (projection > 13 * len_squared) modulationData++;					

					modulationData = _rotate_right(modulationData, 2);

					factor++;
				}
			}

			struct pvrtc_packet* packet = packets + GetMortonNumber(x, y);
			packet->modulationData = modulationData;
		}
	}
}