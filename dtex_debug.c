#include "dtex_debug.h"
#include "dtex_shader.h"

void 
dtex_debug_draw(int id, int pos) {
	float xmin, xmax, ymin, ymax;
	if (pos == 0) {
		xmin = ymin = -1;
		xmax = ymax = 1;
	} else if (pos == 1) {
		xmin = 0;
		xmax = 1;
		ymin = 0;
		ymax = 1;
	} else if (pos == 2) {
		xmin = -1;
		xmax = -0;
		ymin = 0;
		ymax = 1;
	} else if (pos == 3) {
		xmin = -1;
		xmax = -0;
		ymin = -1;
		ymax = -0;
	} else if (pos == 4) {
		xmin = 0;
		xmax = 1;
		ymin = -1;
		ymax = -0;
	}

	float vb[16];

	vb[0] = xmin; vb[1] = ymin; vb[2] = 0; vb[3] = 0;
	vb[4] = xmin; vb[5] = ymax; vb[6] = 0; vb[7] = 1;
	vb[8] = xmax; vb[9] = ymax; vb[10]= 1; vb[11]= 1;
	vb[12]= xmax; vb[13]= ymin; vb[14]= 1; vb[15]= 0;

	dtex_shader_program(DTEX_PROGRAM_NORMAL);

	dtex_shader_begin();
	dtex_shader_draw(vb, id);		
	dtex_shader_end();
}
