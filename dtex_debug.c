#include "dtex_debug.h"
#include "dtex_shader.h"

void 
dtex_debug_draw(int id, int pos) {
	float xmin, xmax, ymin, ymax;
	if (pos == 1) {
		xmin = 0.1f;
		xmax = 0.9f;
		ymin = 0.1f;
		ymax = 0.9f;
	} else if (pos == 2) {
		xmin = -0.9f;
		xmax = -0.1f;
		ymin = 0.1f;
		ymax = 0.9f;
	} else if (pos == 3) {
		xmin = -0.9f;
		xmax = -0.1f;
		ymin = -0.9f;
		ymax = -0.1f;
	} else if (pos == 4) {
		xmin = 0.1f;
		xmax = 0.9f;
		ymin = -0.9f;
		ymax = -0.1f;
	}

	float vb[16];

	vb[0] = xmin; vb[1] = ymin; vb[2] = 0; vb[3] = 0;
	vb[4] = xmin; vb[5] = ymax; vb[6] = 0; vb[7] = 1;
	vb[8] = xmax; vb[9] = ymax; vb[10]= 1; vb[11]= 1;
	vb[12]= xmax; vb[13]= ymin; vb[14]= 1; vb[15]= 0;

	dtex_shader_begin();
	dtex_shader_set_texture(id);
	dtex_shader_draw(vb);		
	dtex_shader_end();
}
