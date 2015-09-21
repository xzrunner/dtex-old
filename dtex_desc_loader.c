#include "dtex_desc_loader.h"



struct ej_sprite_pack*
dtex_load_epe(uint8_t* buf) {
	uint16_t export_n, maxid, tex;
	uint32_t unpack_sz, body_sz;

	STREAM_IMPORT(buf, export_n);
	STREAM_IMPORT(buf, maxid);
	STREAM_IMPORT(buf, tex);
	STREAM_IMPORT(buf, unpack_sz);
	STREAM_IMPORT(buf, body_sz);
}
