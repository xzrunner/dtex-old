#include "dtex_ej_glyph.h"
#include "dtex_shader.h"
#include "dtex_screen.h"

#include <string.h>

void 
dtex_ej_glyph_draw(int texid, float texcoords[], float x, float y, float w, float h) {
	float vb[16];

	float xmin = x, xmax = x + w;
	float ymin = y - h, ymax = y;
	xmin *= 16;
	ymin *= 16;
	xmax *= 16;
	ymax *= 16;
	dtex_screen_trans(&xmin, &ymin);
	dtex_screen_trans(&xmax, &ymax);

	vb[0] = xmin; vb[1] = ymax;
	vb[8] = xmax; vb[9] = ymin;
	vb[4] = xmin; vb[5] = ymin;
	vb[12] = xmax; vb[13] = ymax;

	memcpy(vb+2, texcoords, 2*sizeof(float));
	memcpy(vb+6, texcoords+2, 2*sizeof(float));
	memcpy(vb+10, texcoords+4, 2*sizeof(float));
	memcpy(vb+14, texcoords+6, 2*sizeof(float));

	dtex_shader_texture(texid);
	dtex_shader_draw(vb);
}