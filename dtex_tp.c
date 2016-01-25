#include "dtex_tp.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct tp_node {
	struct dtex_tp_pos pos;

	// pack
	int16_t is_split_y;
	struct tp_node *next, *child, *parent;
	int remain_area, remain_len, remain_space;	
};

struct dtex_tp {
	int w, h;

	struct tp_node* root;

	size_t free_next, free_cap;
	struct tp_node freelist[1];
};

static inline struct tp_node*
_new_node(struct dtex_tp* tp) {
	if (tp->free_next < tp->free_cap) {
		return &tp->freelist[tp->free_next++];
	} else {
		return NULL;
	}
}

static inline void
_init_root(struct dtex_tp* tp, int width, int height) {
	struct tp_node* root = _new_node(tp);
	if (!root) return;
	root->pos.r.xmin = root->pos.r.ymin = 0;
	root->pos.r.xmax = width;
	root->pos.r.ymax = height;
	root->remain_area = width * height;
	root->remain_len = (width > height ? width : height);
	root->remain_space = width;

	struct tp_node* child = _new_node(tp);
	if (!child) return;
	child->pos.r.xmin = child->pos.r.ymin = 0;
	child->pos.r.xmax = width;
	child->pos.r.ymax = height;
	child->remain_area = root->remain_area;
	child->remain_len = root->remain_len;
	child->remain_space = root->remain_space;

	root->child = child;
	tp->root = root;
}

struct dtex_tp* 
dtex_tp_create(int width, int height, int size) {
    size_t node_sz = size * 3 + 2;
	size_t sz = sizeof(struct dtex_tp) + sizeof(struct tp_node) * node_sz;
	struct dtex_tp* tp = (struct dtex_tp*)malloc(sz);
	memset(tp, 0, sz);

	tp->w = width;
	tp->h = height;

	tp->free_next = 0;
	tp->free_cap = node_sz;

	_init_root(tp, width, height);

	return tp;
}

void 
dtex_tp_release(struct dtex_tp* tp) {
	free(tp);
}

void 
dtex_tp_clear(struct dtex_tp* tp) {
	tp->free_next = 0;
	memset(tp->freelist, 0, sizeof(struct tp_node) * tp->free_cap);

	_init_root(tp, tp->w, tp->h);
}

static inline void
_rect_update_remain(struct tp_node* n) {
	struct tp_node* p = n->parent;
	while (p) {
		p->remain_area = 0;
		p->remain_len = 0;
		p->remain_space = 0;
		struct tp_node* c = p->child;
		while (c) {
			if (c->remain_area > p->remain_area) {
				p->remain_area = c->remain_area;
			} if (c->remain_len > p->remain_len) {
				p->remain_len = c->remain_len;
			} if (c->remain_space > p->remain_space) {
				p->remain_space = c->remain_space;
			}  
			c = c->next;
		}
		p = p->parent;
	}
}

static inline int
_node_area(struct tp_node* n) {
	return (n->pos.r.xmax - n->pos.r.xmin) * (n->pos.r.ymax - n->pos.r.ymin);	
}

static inline int
_node_max_length(struct tp_node* n) {
	int16_t w = n->pos.r.xmax - n->pos.r.xmin,
			h = n->pos.r.ymax - n->pos.r.ymin;
	return w > h ? w : h;
}

static inline struct tp_node*
_split_node(struct dtex_tp* tp, struct tp_node* dst, int w, int h) {
	struct tp_node* next = _new_node(tp);
	struct tp_node* child = _new_node(tp);
	struct tp_node* child_next = _new_node(tp);	  
	if (child_next == NULL) {
		return NULL;
	}
	if (dst->is_split_y) {
		next->pos.r.xmin = dst->pos.r.xmin;
		next->pos.r.xmax = dst->pos.r.xmax;
		next->pos.r.ymin = dst->pos.r.ymin+h;
		next->pos.r.ymax = dst->pos.r.ymax;
		dst->pos.r.ymax = next->pos.r.ymin;
		next->is_split_y = 1;

		child->pos.r.ymin = dst->pos.r.ymin;
		child->pos.r.ymax = dst->pos.r.ymax;
		child->pos.r.xmin = dst->pos.r.xmin;
		child->pos.r.xmax = dst->pos.r.xmin+w;
		child->is_split_y = 0;

		child_next->pos.r.ymin = dst->pos.r.ymin;
		child_next->pos.r.ymax = dst->pos.r.ymax;
		child_next->pos.r.xmin = child->pos.r.xmax;
		child_next->pos.r.xmax = dst->pos.r.xmax;
		child_next->is_split_y = 0;
	} else {
		next->pos.r.ymin = dst->pos.r.ymin;
		next->pos.r.ymax = dst->pos.r.ymax;
		next->pos.r.xmin = dst->pos.r.xmin+w;
		next->pos.r.xmax = dst->pos.r.xmax;
		dst->pos.r.xmax = next->pos.r.xmin;
		next->is_split_y = 0;

		child->pos.r.xmin = dst->pos.r.xmin;
		child->pos.r.xmax = dst->pos.r.xmax;
		child->pos.r.ymin = dst->pos.r.ymin;
		child->pos.r.ymax = dst->pos.r.ymin+h;
		child->is_split_y = 1;

		child_next->pos.r.xmin = dst->pos.r.xmin;
		child_next->pos.r.xmax = dst->pos.r.xmax;
		child_next->pos.r.ymin = child->pos.r.ymax;
		child_next->pos.r.ymax = dst->pos.r.ymax;
		child_next->is_split_y = 1;
	}
	dst->next = next;
	next->parent = dst->parent;
	dst->child = child;
	child->parent = dst;
	child->next = child_next;
	child_next->parent = dst;

	// remain area
	next->remain_area = _node_area(next);
	child_next->remain_area = _node_area(child_next);
	child->remain_area = 0;
	dst->remain_area = child_next->remain_area;
	// remain len
	next->remain_len = _node_max_length(next);
	child_next->remain_len = _node_max_length(child_next);
	child->remain_len = 0;
	dst->remain_len = child_next->remain_len;  
	// remain_space
	if (dst->is_split_y) {
		next->remain_space = next->pos.r.ymax - next->pos.r.ymin;
		child_next->remain_space = child_next->pos.r.xmax - child_next->pos.r.xmin;
		child->remain_space = 0;
		dst->remain_space = child_next->remain_space;
	} else {
		next->remain_space = next->pos.r.xmax - next->pos.r.xmin;
		child_next->remain_space = child_next->pos.r.ymax - child_next->pos.r.ymin;
		child->remain_space = 0;
		dst->remain_space = child_next->remain_space;
	}
	_rect_update_remain(dst);

	return child;
}

static inline int
_rect_room_enough(struct tp_node* n, int w, int h) {
	if ((w <= n->remain_space || h <= n->remain_space) &&
		w * h <= n->remain_area &&
		w <= n->remain_len && h <= n->remain_len) {
		return 1;
	} else {
		return 0;
	}
}

static inline struct tp_node*
_insert(struct dtex_tp* tp, struct tp_node* dst, int w, int h, bool can_rotate) {
	assert(dst != NULL);
	int16_t dw = dst->pos.r.xmax - dst->pos.r.xmin,
	        dh = dst->pos.r.ymax - dst->pos.r.ymin;
	if ((w > dw && h > dh) || (w > dh && h > dw)) {
		return NULL;
	}

	if (dst->child == NULL) {
		if (dst->pos.ud) {
			return NULL;
		}
		if (w <= dw && h <= dh) {
			return _split_node(tp, dst, w, h);
		} else if (w <= dh && h <= dw && can_rotate) {
			struct tp_node* n = _split_node(tp, dst, h, w);
			if (n) {
				n->pos.is_rotated = true;
			}
			return n;
		} else {
			return NULL;
		}
	} else {
		struct tp_node* next = dst->child;
		while (next) {
			struct tp_node* node = NULL;
			if (_rect_room_enough(next, w, h)) {
				node = _insert(tp, next, w, h, can_rotate);
			}
			if (node) {
				return node;
			} else {
				next = next->next;
			}
		}
		return NULL;
	}
}

struct dtex_tp_pos* 
dtex_tp_add(struct dtex_tp* tp, int width, int height, bool can_rotate) {
	struct tp_node* node = _insert(tp, tp->root, width, height, can_rotate);
	return &node->pos;
}

void 
dtex_tp_get_size(struct dtex_tp* tp, int* width, int* height) {
	*width = tp->w;
	*height = tp->h;
}

int 
dtex_tp_get_free_space(struct dtex_tp* tp) {
	return tp->root->remain_area;
}