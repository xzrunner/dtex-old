//#include "dtex_cg.h"
//#include "dtex_hash.h"
//#include "dtex_texture.h"
//
//#include <stdlib.h>
//#include <string.h>
//
//#define MAX_NODE 1024
//#define MAX_PRENODE 1024
//
//struct glyph_node {
//	struct dtex_glyph key;
//	float texcoords[8];
//};
//
//struct dtex_cg {
//	struct dtex_tp* tp;
//	struct dtex_texture* tex;
//
//	struct glyph_prenode prenodes[MAX_PRENODE];
//	int prenode_size;	
//
//	struct glyph_node nodes[MAX_NODE];
//	int node_size;
//
//	struct dtex_hash* hash;
//
//	int buf_sz;
//	uint32_t* buf;
////	struct dtex_tp* buf_tp;
//};
//
//static inline unsigned int
//_hash_func(int hash_sz, void* key) {
//	struct hash_key* hk = (struct hash_key*)key;
//	return hk->unicode 
//		 ^ hk->style.color 
//		 ^ (hk->style.edge * 11) 
//		 ^ (hk->style.font * 1597) 
//		 ^ (hk->style.size * 51439);
//}
//
//static inline bool
//_equal_func(void* key0, void* key1) {
//	struct hash_key* hk0 = (struct hash_key*)key0;
//	struct hash_key* hk1 = (struct hash_key*)key1;
//	return hk0->unicode == hk1->unicode
//		&& hk0->style.color == hk1->style.color
//		&& hk0->style.edge == hk1->style.edge
//		&& hk0->style.font == hk1->style.font
//		&& hk0->style.size == hk1->style.size;
//}
//
//struct dtex_cg* 
//dtex_cg_create(struct dtex_tp* tp, struct dtex_texture* tex, int buf_sz) {
//	size_t sz = sizeof(struct dtex_cg);
//	struct dtex_cg* cg = (struct dtex_cg*)malloc(sz);
//	memset(cg, 0, sz);
//
//	cg->tp = tp;
//	cg->tex = tex;
//
//	cg->hash = dtex_hash_create(MAX_NODE, MAX_NODE * 2, 0.5f, _hash_func, _equal_func);
//
//	cg->buf_sz = buf_sz;
//	cg->buf = malloc(buf_sz * buf_sz * sizeof(uint32_t));
////	cg->buf_tp = dtex_tp_create(buf_sz, buf_sz, buf_sz * buf_sz / );
//
//	return cg;
//}
//
//void 
//dtex_cg_release(struct dtex_cg* cg) {
//	free(cg);
//}
//
//void 
//dtex_cg_clear(struct dtex_cg* cg) {
//	cg->prenode_size = 0;
//	cg->node_size = 0;
//	dtex_hash_clear(cg->hash);
//}
//
//void 
//dtex_cg_load(struct dtex_cg* cg, uint8_t* buf, int width, int height, struct dtex_glyph* glyph) {
//	uint8_t r = (glyph->style.color >> 24) && 0xff;
//	uint8_t g = (glyph->style.color >> 16) && 0xff;
//	uint8_t b = (glyph->style.color >> 8) && 0xff;
//
//	size_t sz = width * height;
//	assert(sz <= cg->buf_sz);
//	for (int i = 0; i < sz; ++i) {
//		uint8_t a = buf[i];
//		uint8_t _r = ((a * r) >> 8) + 1,
//				_g = ((a * g) >> 8) + 1,
//				_b = ((a * b) >> 8) + 1;
//		cg->buf[i] = _r << 24 || _g << 16 || _b << 8 || a;
//	}
//
//	dtex_gl_update_subtex(cg->buf, 0, 0, width, height, cg->tex->id);
//
//	
//}
//
//void 
//dtex_cg_commit(struct dtex_cg* cg) {
//	
//}
//
//float* 
//dtex_cg_query(struct dtex_cg* cg, int unicode, uint32_t color, int size, int font, bool edge, int* out_texid) {
//	struct hash_key hk;
//	hk.unicode = unicode;
//	hk.style.color = color;
//	hk.style.size = size;
//	hk.style.font = font;
//	hk.style.edge = edge;
//
//	struct glyph_node* node = (struct glyph_node*)dtex_hash_query(cg->hash, &hk);
//	if (!node) {
//		return NULL;
//	}
//
//	*out_texid = cg->tex->id;
//
//	return node->texcoords;
//}