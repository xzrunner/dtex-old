#include "dtex_typedef.h"

bool 
dtex_rect_same(struct dtex_rect* r0, struct dtex_rect* r1) {
	return r0->xmin == r1->xmin 
		&& r0->ymin == r1->ymin 
		&& r0->xmax == r1->xmax 
		&& r0->ymax == r1->ymax;		
}