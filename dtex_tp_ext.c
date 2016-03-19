#include "dtex_tp_ext.h"
#include "dtex_math.h"
#include "dtex_tp.h"

#include <ds_array.h>

#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <string.h>

static inline int
_compare_width(const void *arg1, const void *arg2) {
	struct dtex_tp_rect *node1, *node2;

	node1 = *((struct dtex_tp_rect**)(arg1));
	node2 = *((struct dtex_tp_rect**)(arg2));

	if (node1->w > node2->w) {
		return -1;
	} else if (node1->w < node2->w) {
		return 1;
	} else {
		return 0;
	}
}

static inline int
_compare_max_edge(const void *arg1, const void *arg2) {
	struct dtex_tp_rect *node1, *node2;

	node1 = *((struct dtex_tp_rect**)(arg1));
	node2 = *((struct dtex_tp_rect**)(arg2));

	int16_t w1 = node1->w, h1 = node1->h;
	int16_t w2 = node2->w, h2 = node2->h;
	int16_t long1, long2, short1, short2;
	if (w1 > h1) {
		long1 = w1;
		short1 = h1;
	} else {
		long1 = h1;
		short1 = w1;
	}
	if (w2 > h2) {
		long2 = w2;
		short2 = h2;
	} else {
		long2 = h2;
		short2 = w2;
	}

	if (long1 > long2) {
		return -1;
	} else if (long1 < long2) {
		return 1;
	} else {
		if (short1 > short2) return -1;
		else if (short1 < short2) return 1;
		else return 0;
	}
}

static inline int 
_cal_area(struct dtex_tp_rect** rects, int sz) {
	int area = 0;
	for (int i = 0; i < sz; ++i) {
		struct dtex_tp_rect* r = rects[i];
		area += r->w * r->h;
	}
	return area;
}

struct ds_array* 
dtex_packer_square_multi(struct dtex_tp_rect** rects, int sz) {
	static const float AREA_SCALE_LIMIT = 0.55f;
	//	static const float AREA_LIMIT = 64 * 64;
	static const int EDGE_LIMIT = 256;

	qsort(rects, sz, sizeof(struct dtex_tp_rect*), _compare_width);

	int curr_sz = sz;
	struct dtex_tp_rect* curr_list[sz];
	memcpy(curr_list, rects, sizeof(struct dtex_tp_rect*) * sz);

	int area = _cal_area(curr_list, sz);
	int edge = next_p2((int)ceil(sqrt((float)area)));

	struct ds_array* packers = ds_array_create(8, sizeof(struct dtex_tp*));
	while (curr_sz != 0) {
		int success_sz = 0, fail_sz = 0;
		struct dtex_tp_rect *success_list[sz], *fail_list[sz];

		int packer_sz = ds_array_size(packers);

		struct dtex_tp* tp = dtex_tp_create(edge, edge, sz);
		for (int i = 0; i < curr_sz; ++i) {
			struct dtex_tp_rect* r = curr_list[i];
			struct dtex_tp_pos* pos = dtex_tp_add(tp, r->w, r->h, false);
			if (pos) {
				assert(!pos->is_rotated);
				r->dst_packer_idx = packer_sz;
				r->dst_pos = pos;
				success_list[success_sz++] = r;
			} else {
				assert(r->dst_packer_idx == -1);

				r->dst_packer_idx = -1;
				r->dst_pos = NULL;
				fail_list[fail_sz++] = r;
			}
		}

		int used_area = _cal_area(success_list, success_sz);
		float fill = (float)used_area / (edge*edge);
		if (success_sz == 0) {
			edge *= 2;
			dtex_tp_release(tp);
		} else {
			if (fill > AREA_SCALE_LIMIT || edge <= EDGE_LIMIT) {
				memcpy(curr_list, fail_list, sizeof(struct dtex_tp_rect*) * fail_sz);
				curr_sz = fail_sz;
				area -= used_area;
				edge = next_p2((int)ceil(sqrt((float)area)));
				ds_array_add(packers, tp);
			} else {
				edge /= 2;
				dtex_tp_release(tp);
#ifdef _DEBUG
				for (int i = 0; i < curr_sz; ++i) {
					struct dtex_tp_rect* r = curr_list[i];
					r->dst_packer_idx = -1;
				}
#endif // _DEBUG
			}
		}
	}

	return packers;
}