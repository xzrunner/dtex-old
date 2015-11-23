#include "dtex_debug.h"
#include "dtex_gl.h"
#include "dtex_shader.h"

#include <assert.h>

void 
dtex_debug_draw(unsigned int texid) {
	dtex_debug_draw_with_pos(texid, 0, 0, 1, 1);
}

void 
dtex_debug_draw_with_pos(unsigned int texid, 
						 float xmin, 
						 float ymin, 
						 float xmax, 
						 float ymax) {
	assert(dtex_gl_is_texture(texid));

	dtex_shader_program(PROGRAM_NORMAL);

	float vb[16];

	vb[2] = 0, vb[3] = 0;
	vb[6] = 0, vb[7] = 1;
	vb[10] = 1, vb[11] = 1;
	vb[14] = 1, vb[15] = 0;

	vb[0] = xmin, vb[1] = ymin;
	vb[4] = xmin, vb[5] = ymax;
	vb[8] = xmax, vb[9] = ymax;
	vb[12] = xmax, vb[13] = ymin;

	dtex_shader_begin();
	dtex_shader_set_texture(texid);
	dtex_shader_draw(vb);		
	dtex_shader_end();
}

#ifndef USED_IN_EDITOR

void 
dtex_debug_draw_ej(int uid_3rd, int pos) {
	struct ej_vertex_pack vb[4];

	float xmin, xmax, ymin, ymax;
	if (pos == 1) {
		xmin = 1.1f;
		xmax = 1.9f;
		ymin = -0.9f;
		ymax = -0.1f;
	} else if (pos == 2) {
		xmin = 0.1f;
		xmax = 0.9f;
		ymin = -0.9f;
		ymax = -0.1f;
	} else if (pos == 3) {
		xmin = 0.1f;
		xmax = 0.9f;
		ymin = -1.9f;
		ymax = -1.1f;
	} else if (pos == 4) {
		xmin = 1.1f;
		xmax = 1.9f;
		ymin = -1.9f;
		ymax = -1.1f;
	}

	vb[0].vx = xmin;	vb[0].vy = ymin;
	vb[1].vx = xmin;	vb[1].vy = ymax;
	vb[2].vx = xmax;	vb[2].vy = ymax;
	vb[3].vx = xmax;	vb[3].vy = ymin;

	vb[0].tx = 0;	vb[0].ty = 0;
	vb[1].tx = 0;	vb[1].ty = 0xffff;
	vb[2].tx = 0xffff;	vb[2].ty = 0xffff;
	vb[3].tx = 0xffff;	vb[3].ty = 0;

	ej_shader_program(PROGRAM_PICTURE, NULL);
	ej_shader_texture(uid_3rd, 0);
	ej_shader_draw(vb, 0xffffffff, 0, 0xff0000ff, 0x00ff00ff, 0x0000ffff);
}

#endif // USED_IN_EDITOR