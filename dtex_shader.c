#include "dtex_shader.h"

static void (*PROGRAM)(int n);
static void (*BLEND)(int mode);
static void (*SET_TEXTURE)(int id);
static int (*GET_TEXTURE)();
static void (*SET_TARGET)(int id);
static int (*GET_TARGET)();
static void (*DRAW_BEGIN)();
static void (*DRAW)(const float vb[16], int texid);
static void (*DRAW_END)();
static void (*DRAW_FLUSH)();
static void (*SCISSOR_ENABLE)();
static void (*SCISSOR_DISABLE)();

void 
dtex_shader_init(void (*program)(int n),
				 void (*blend)(int mode),
				 void (*set_texture)(int id),
				 int (*get_texture)(),
				 void (*set_target)(int id),
				 int (*get_target)(),
				 void (*draw_begin)(),
				 void (*draw)(const float vb[16], int texid),
				 void (*draw_end)(),
				 void (*draw_flush)(),
				 void (*scissor_enable)(),
				 void (*scissor_disable)()) {
	PROGRAM = program;
	BLEND = blend;
	SET_TEXTURE = set_texture;
	GET_TEXTURE = get_texture;
	SET_TARGET = set_target;
	GET_TARGET = get_target;
	DRAW_BEGIN = draw_begin;
	DRAW = draw;
	DRAW_END = draw_end;
	DRAW_FLUSH = draw_flush;
	SCISSOR_ENABLE = scissor_enable;
	SCISSOR_DISABLE = scissor_disable;
}

void 
dtex_shader_program(int n) {
	PROGRAM(n);
}

void 
dtex_shader_blend(int mode) {
	BLEND(mode);
}

void 
dtex_shader_set_texture(int id) {
	SET_TEXTURE(id);
}

int 
dtex_shader_get_texture() {
	return GET_TEXTURE();
}

void 
dtex_shader_set_target(int id) {
	SET_TARGET(id);
}

int 
dtex_shader_get_target() {
	return GET_TARGET();
}

void 
dtex_shader_begin() {
	DRAW_BEGIN();
}

void 
dtex_shader_draw(const float vb[16], int texid) {
	DRAW(vb, texid);
}

//void dtex_shader_draw_triangle(const float* vb, int count);

void 
dtex_shader_end() {
	DRAW_END();
}

void 
dtex_shader_flush() {
	DRAW_FLUSH();
}

void 
dtex_shader_scissor(bool enable) {
	if (enable) {
		if (SCISSOR_ENABLE) {
			SCISSOR_ENABLE();
		}
	} else {
		if (SCISSOR_DISABLE) {
			SCISSOR_DISABLE();
		}
	}
}