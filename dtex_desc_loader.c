#include "dtex_desc_loader.h"
#include "dtex_utility.h"
#include "dtex_log.h"
#include "dtex_stream_import.h"
#include "dtex_package.h"

#include "dtex_rrp.h"
#include "dtex_pts.h"
#include "dtex_rrr.h"
#include "dtex_b4r.h"

#include <stdlib.h>
#include <string.h>

// static inline int
// _comp_export(const void* a, const void* b) {
// 	const struct export* aa = a;
// 	const struct export* bb = b;
// 	return strcmp(aa->name,bb->name);
// }
// 
// static inline void
// _sort_ep(struct ejoypic* ep) {
// 	qsort(ep->export, ep->export_n, sizeof(struct export), _comp_export);
// }

static int
_comp_export(const void *a, const void *b) {
	const struct export_name* aa = a;
	const struct export_name* bb = b;
	return strcmp(aa->name,bb->name);
}

struct ej_sprite_pack*
dtex_load_epe(struct dtex_import_stream* is, struct dtex_package* pkg) {
	uint16_t export_n = dtex_import_uint16(is);
	uint16_t maxid = dtex_import_uint16(is);
	uint16_t tex = dtex_import_uint16(is);
	uint32_t unpack_sz = dtex_import_uint32(is);
	uint32_t body_sz = dtex_import_uint32(is);

	pkg->export_names = malloc(sizeof(struct export_name) * export_n);
	memset(pkg->export_names, 0, sizeof(struct export_name) * export_n);
	pkg->export_size = 0;

	for (int i = 0; i < export_n; ++i) {
		uint16_t id = dtex_import_uint16(is);
		const char* name = dtex_import_string(is);

		struct export_name* ep = &pkg->export_names[pkg->export_size++];
		ep->name = name;
		ep->id = id;
	}

	qsort(pkg->export_names, pkg->export_size, sizeof(struct export_name), _comp_export);

	struct ej_sprite_pack* ej_pkg = ej_pkg_import((void*)is->stream, body_sz, tex, maxid, unpack_sz);
	return ej_pkg;
}

struct dtex_rrp* 
dtex_load_rrp(struct dtex_import_stream* is) {
// 	uint32_t cap = buf[0] | buf[1]<<8 | buf[2]<<16 | buf[3]<<24;
// 	struct dtex_rrp* rrp = dtex_rrp_create(buf + 4, sz - 4, cap);
// 	if (rrp == NULL) {
// 		dtex_fault("Error create rrp.\n");
// 	}
// 	return rrp;

	return NULL;
}

struct dtex_pts* 
dtex_load_pts(uint8_t* buf, size_t sz) {
// 	uint32_t cap = buf[0] | buf[1]<<8 | buf[2]<<16 | buf[3]<<24;
// 	struct dtex_pts* pts = dtex_pts_create(buf + 4, sz - 4, cap);
// 	if (pts == NULL) {
// 		dtex_fault("Error create pts.\n");
// 	}
// 	return pts;

	return NULL;
}

struct dtex_rrr* 
dtex_load_rrr(struct dtex_import_stream* is) {
// 	uint32_t cap = buf[0] | buf[1]<<8 | buf[2]<<16 | buf[3]<<24;
// 	struct dtex_rrr* rrr = dtex_rrr_create(buf + 4, sz - 4, cap);
// 	if (rrr == NULL) {
// 		dtex_fault("Error create rrr.\n");
// 	}
// 	// dtex_rrr_preload_to_pkg(rrr, pkg);
// 	return rrr;

	return NULL;
}

struct dtex_b4r* 
dtex_load_b4r(struct dtex_import_stream* is) {
// 	uint32_t cap = buf[0] | buf[1]<<8 | buf[2]<<16 | buf[3]<<24;
// 	struct dtex_b4r* b4r = dtex_b4r_create(buf + 4, sz - 4, cap);
// 	if (b4r == NULL) {
// 		dtex_fault("Error create b4r.\n");
// 	}
// 	// dtex_b4r_preload_to_pkg(b4r, pkg);
// 	return b4r;

	return NULL;
}