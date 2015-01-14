#include "dtex_packer.h"

//#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct dp_pack {
	int16_t is_split_y;
	struct dp_node *next, *child, *parent;
	int remain_area, remain_len, remain_space;	
};

struct dp_node {
	struct dp_position pos;
	struct dp_pack pack;
};

struct dtex_packer {
	struct dp_node* root;

	int capacity;
	struct dp_node** node_list;
	int node_sz;
};

static inline struct dp_node*
_new_node(struct dtex_packer* packer) {
	if (packer->node_sz < packer->capacity) {
		return packer->node_list[packer->node_sz++];
	} else {
		return NULL;
	}
}

static inline void
_init_root(struct dtex_packer* packer, int width, int height) {
	struct dp_node* root = _new_node(packer);
	if (!root) return;
	root->pos.r.xmin = root->pos.r.ymin = 0;
	root->pos.r.xmax = width;
	root->pos.r.ymax = height;
	root->pack.remain_area = width * height;
	root->pack.remain_len = (width > height ? width : height);
	root->pack.remain_space = width;

	struct dp_node* child = _new_node(packer);
	if (!child) return;
	child->pos.r.xmin = child->pos.r.ymin = 0;
	child->pos.r.xmax = width;
	child->pos.r.ymax = height;
	child->pack.remain_area = root->pack.remain_area;
	child->pack.remain_len = root->pack.remain_len;
	child->pack.remain_space = root->pack.remain_space;

	root->pack.child = child;
	packer->root = root;
}

struct dtex_packer* 
dtexpacker_create(int width, int height, int size) {
    size_t sz = size * 3 + 2;
	size_t ptr_sz = sizeof(struct dp_node*) * sz;
	size_t node_sz = sizeof(struct dp_node) * sz;
	size_t packer_sz = sizeof(struct dtex_packer) + ptr_sz + node_sz;
	struct dtex_packer* packer = (struct dtex_packer*)malloc(packer_sz);
	memset(packer, 0, packer_sz);

	packer->capacity = sz;

	void* ptr = packer + 1;
	packer->node_list = (struct dp_node**)ptr;
	struct dp_node* first_node = (struct dp_node*)((intptr_t)(packer->node_list) + ptr_sz);
	for (int i = 0; i < sz; ++i) {
		struct dp_node* ptr = first_node + i;
		packer->node_list[i] = ptr;
	}

	_init_root(packer, width, height);
	return packer;
}

void 
dtexpacker_release(struct dtex_packer* packer) {
	if (packer) {
		free(packer);
	}
}

static inline void
_rect_update_remain(struct dp_node* n) {
	struct dp_node* p = n->pack.parent;
	while (p) {
		p->pack.remain_area = 0;
		p->pack.remain_len = 0;
		p->pack.remain_space = 0;
		struct dp_node* c = p->pack.child;
		while (c) {
			if (c->pack.remain_area > p->pack.remain_area) {
				p->pack.remain_area = c->pack.remain_area;
			} if (c->pack.remain_len > p->pack.remain_len) {
				p->pack.remain_len = c->pack.remain_len;
			} if (c->pack.remain_space > p->pack.remain_space) {
				p->pack.remain_space = c->pack.remain_space;
			}  
			c = c->pack.next;
		}
		p = p->pack.parent;
	}
}

static inline int
_node_area(struct dp_node* n) {
	return (n->pos.r.xmax - n->pos.r.xmin) * (n->pos.r.ymax - n->pos.r.ymin);	
}

static inline int
_node_max_length(struct dp_node* n) {
	int16_t w = n->pos.r.xmax - n->pos.r.xmin,
			h = n->pos.r.ymax - n->pos.r.ymin;
	return w > h ? w : h;
}

static inline struct dp_node*
_split_node(struct dtex_packer* packer, struct dp_node* dst, int w, int h) {
	struct dp_node* next = _new_node(packer);
	struct dp_node* child = _new_node(packer);
	struct dp_node* child_next = _new_node(packer);	  
	if (child_next == NULL) {
		return NULL;
	}
	if (dst->pack.is_split_y) {
		next->pos.r.xmin = dst->pos.r.xmin;
		next->pos.r.xmax = dst->pos.r.xmax;
		next->pos.r.ymin = dst->pos.r.ymin+h;
		next->pos.r.ymax = dst->pos.r.ymax;
		dst->pos.r.ymax = next->pos.r.ymin;
		next->pack.is_split_y = 1;

		child->pos.r.ymin = dst->pos.r.ymin;
		child->pos.r.ymax = dst->pos.r.ymax;
		child->pos.r.xmin = dst->pos.r.xmin;
		child->pos.r.xmax = dst->pos.r.xmin+w;
		child->pack.is_split_y = 0;

		child_next->pos.r.ymin = dst->pos.r.ymin;
		child_next->pos.r.ymax = dst->pos.r.ymax;
		child_next->pos.r.xmin = child->pos.r.xmax;
		child_next->pos.r.xmax = dst->pos.r.xmax;
		child_next->pack.is_split_y = 0;
	} else {
		next->pos.r.ymin = dst->pos.r.ymin;
		next->pos.r.ymax = dst->pos.r.ymax;
		next->pos.r.xmin = dst->pos.r.xmin+w;
		next->pos.r.xmax = dst->pos.r.xmax;
		dst->pos.r.xmax = next->pos.r.xmin;
		next->pack.is_split_y = 0;

		child->pos.r.xmin = dst->pos.r.xmin;
		child->pos.r.xmax = dst->pos.r.xmax;
		child->pos.r.ymin = dst->pos.r.ymin;
		child->pos.r.ymax = dst->pos.r.ymin+h;
		child->pack.is_split_y = 1;

		child_next->pos.r.xmin = dst->pos.r.xmin;
		child_next->pos.r.xmax = dst->pos.r.xmax;
		child_next->pos.r.ymin = child->pos.r.ymax;
		child_next->pos.r.ymax = dst->pos.r.ymax;
		child_next->pack.is_split_y = 1;
	}
	dst->pack.next = next;
	next->pack.parent = dst->pack.parent;
	dst->pack.child = child;
	child->pack.parent = dst;
	child->pack.next = child_next;
	child_next->pack.parent = dst;

	// remain area
	next->pack.remain_area = _node_area(next);
	child_next->pack.remain_area = _node_area(child_next);
	child->pack.remain_area = 0;
	dst->pack.remain_area = child_next->pack.remain_area;
	// remain len
	next->pack.remain_len = _node_max_length(next);
	child_next->pack.remain_len = _node_max_length(child_next);
	child->pack.remain_len = 0;
	dst->pack.remain_len = child_next->pack.remain_len;  
	// remain_space
	if (dst->pack.is_split_y) {
		next->pack.remain_space = next->pos.r.ymax - next->pos.r.ymin;
		child_next->pack.remain_space = child_next->pos.r.xmax - child_next->pos.r.xmin;
		child->pack.remain_space = 0;
		dst->pack.remain_space = child_next->pack.remain_space;
	} else {
		next->pack.remain_space = next->pos.r.xmax - next->pos.r.xmin;
		child_next->pack.remain_space = child_next->pos.r.ymax - child_next->pos.r.ymin;
		child->pack.remain_space = 0;
		dst->pack.remain_space = child_next->pack.remain_space;
	}
	_rect_update_remain(dst);

	return child;
}

static inline int
_rect_room_enough(struct dp_node* n, int w, int h) {
	if ((w <= n->pack.remain_space || h <= n->pack.remain_space) &&
		w * h <= n->pack.remain_area &&
		w <= n->pack.remain_len && h <= n->pack.remain_len) {
		return 1;
	} else {
		return 0;
	}
}

static inline struct dp_node*
_insert(struct dtex_packer* packer, struct dp_node* dst, int w, int h) {
	assert(dst != NULL);
	int16_t dw = dst->pos.r.xmax - dst->pos.r.xmin,
	        dh = dst->pos.r.ymax - dst->pos.r.ymin;
	if ((w > dw && h > dh) || (w > dh && h > dw)) {
		return NULL;
	}

	if (dst->pack.child == NULL) {
		if (dst->pos.ud) {
			return NULL;
		}
		if (w <= dw && h <= dh) {
			return _split_node(packer, dst, w, h);
		} else if (w <= dh && h <= dw) {
			struct dp_node* n = _split_node(packer, dst, h, w);
			if (n) {
				n->pos.is_rotated = 1;
			}
			return n;
		} else {
			return NULL;
		}
	} else {
		struct dp_node* next = dst->pack.child;
		while (next) {
			struct dp_node* node = NULL;
			if (_rect_room_enough(next, w, h)) {
				node = _insert(packer, next, w, h);
			}
			if (node) {
				return node;
			} else {
				next = next->pack.next;
			}
		}
		return NULL;
	}
}

struct dp_position* 
dtexpacker_add(struct dtex_packer* packer, int width, int height) {
	struct dp_node* node =  _insert(packer, packer->root, width, height);
	return &node->pos;
}