#include "dtex_shader.h"
#include "dtex_log.h"
#include "dtex_statistics.h"

#include "dtex_gl.h"
#include <assert.h>

#include <opengl.h>

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// todo: should different from engine
#define ATTRIB_VERTEX		10
#define ATTRIB_TEXTCOORD	11
#define ATTRIB_COLOR		12
#define ATTRIB_ADDITIVE		13

#define MAX_COMMBINE 1024

#define BUFFER_OFFSET(i) ((char*)NULL + (i))

struct render_state {
	int blend;
	int activeprog;
	int texture;
	int target;
	int object;
	float vb[24 * MAX_COMMBINE];
};

static struct render_state* RS = NULL;

struct program {
	GLuint prog;
	GLint sampler0, sampler1;
};

static struct program PROG[7];
#define PROGRAM_TOTAL (sizeof(PROG)/sizeof(PROG[0]))

static GLuint VERTEX_BUFFER = 0;
static GLuint INDEX_BUFFER = 0;

// todo
static const uint32_t MULTI_COL = 0xffffffff;
static const uint32_t ADD_COL = 0;

static inline void
_rs_init() {
	struct render_state* rs = malloc(sizeof(*rs));
// 	rs->edge = 0;
// 	rs->color = 0xffffffff;
// 	rs->additive = 0;
	rs->blend = GL_ONE_MINUS_SRC_ALPHA;
	rs->activeprog = -1;
	rs->texture = 0;
	rs->target = 0;
	rs->object = 0;

	glBlendFunc(GL_ONE, rs->blend);

	glGenBuffers(1, &INDEX_BUFFER);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, INDEX_BUFFER);

	size_t size = MAX_COMMBINE * 6 * sizeof(GLushort);
	GLushort* idxs = malloc(size);
	int i;
	for (i=0;i<MAX_COMMBINE;i++) {
		idxs[i*6] = i*4;
		idxs[i*6+1] = i*4+1;
		idxs[i*6+2] = i*4+2;
		idxs[i*6+3] = i*4;
		idxs[i*6+4] = i*4+2;
		idxs[i*6+5] = i*4+3;
	}
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, idxs, GL_STATIC_DRAW);
	free(idxs);

	RS = rs;
}

static inline GLuint
_compile(const char* source, int type) {
	GLint status;

	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

	if (status == 0) {
		char buf[1024];
		glGetShaderInfoLog(shader, 1024, NULL, buf);
		glDeleteShader(shader);
		dtex_fault("compile failed:%s\n, source:\n %s\n", buf, source);
	}
	return shader;
}

static inline void
_link(struct program* p) {
	GLint status;
	glLinkProgram(p->prog);

	glGetProgramiv(p->prog, GL_LINK_STATUS, &status);
	if (status == 0) {
		char buf[1024];
		glGetProgramInfoLog(p->prog, 1024, NULL, buf);
		dtex_fault("link failed:%s\n", buf);
	}
}

static inline void
_program_init(struct program* p, const char* VS, const char* FS) {
	// Create shader program.
	p->prog = glCreateProgram();

	GLuint fs = _compile(FS, GL_FRAGMENT_SHADER);
	if (fs == 0) {
		dtex_fault("Can't compile fragment shader");
	} else {
		glAttachShader(p->prog, fs);
	}

	GLuint vs = _compile(VS, GL_VERTEX_SHADER);
	if (vs == 0) {
		dtex_fault("Can't compile vertex shader");
	} else {
		glAttachShader(p->prog, vs);
	}

	glBindAttribLocation(p->prog, ATTRIB_VERTEX, "position");
	glBindAttribLocation(p->prog, ATTRIB_TEXTCOORD, "texcoord");
	glBindAttribLocation(p->prog, ATTRIB_COLOR, "color");
	glBindAttribLocation(p->prog, ATTRIB_ADDITIVE, "additive");

	_link(p);

	p->sampler0 = glGetUniformLocation(p->prog, "texture0");
	if (p == &PROG[PROGRAM_ETC1])
		p->sampler1 = glGetUniformLocation(p->prog, "texture1");
	else
		p->sampler1 = 0;

	glDetachShader(p->prog, fs);
	glDeleteShader(fs);
	glDetachShader(p->prog, vs);
	glDeleteShader(vs);
}

void 
dtex_shader_load() {
	if (RS) {
		return;
	}

#if 0
#define FLOAT_PRECISION \
	"#ifdef GL_FRAGMENT_PRECISION_HIGH  \n" \
	"precision highp float;  \n" \
	"#else  \n" \
	"precision lowp float;  \n" \
	"#endif  \n"
#else
#define FLOAT_PRECISION \
	"#version 100 \nprecision highp float;  \n"
#endif

	static const char * sprite_vs =
		FLOAT_PRECISION
		"\n"
		"attribute vec4 position;  \n"
		"attribute vec2 texcoord;  \n"
		"attribute vec4 color;     \n"
		"attribute vec4 additive;     \n"
		"\n"
		"varying vec2 v_texcoord;  \n"
		"varying vec4 v_fragmentColor;  \n"
		"varying vec4 v_fragmentAddi; \n"
		"\n"
		"void main()  \n"
		"{  \n"
		"  gl_Position = position;  \n"
		"  v_fragmentColor = color / 255.0; \n"
		"  v_fragmentAddi = additive / 255.0; \n"
		"  v_texcoord = texcoord;  \n"
		"}  \n"
		;

	static const char * sprite_fs =
		FLOAT_PRECISION
		"\n"
		"varying vec4 v_fragmentColor; \n"
		"varying vec4 v_fragmentAddi; \n"
		"varying vec2 v_texcoord;  \n"
		"uniform sampler2D texture0;  \n"
		"\n"
		"void main()  \n"
		"{  \n"
		"  vec4 tmp = texture2D(texture0, v_texcoord);  \n"
		"  gl_FragColor.xyz = tmp.xyz * v_fragmentColor.xyz;  \n"
		"  gl_FragColor.w = tmp.w;    \n"
		"  gl_FragColor *= v_fragmentColor.w;  \n"
		"  gl_FragColor.xyz += v_fragmentAddi.xyz * tmp.w;  \n"
		"}  \n"
		;

	static const char* etc1_fs =
		FLOAT_PRECISION

		"varying vec4 v_fragmentColor; \n"
		"varying vec4 v_fragmentAddi; \n"
		"varying vec2 v_texcoord;  \n"
		"uniform sampler2D texture0;  \n"
		"uniform sampler2D texture1;  \n"
		"\n"
		"void main()  \n"
		"{  \n"  

		// todo
//		"  v_texcoord.y = 1 - v_texcoord.y;  \n"

		"  vec4 tmp = texture2D(texture0, v_texcoord);  \n"
		"  tmp.w = texture2D(texture1, v_texcoord).r;  \n"
		"  gl_FragColor.xyz = tmp.xyz * v_fragmentColor.xyz;  \n"
		"  gl_FragColor.w = tmp.w;    \n"
		"  gl_FragColor *= v_fragmentColor.w;  \n"
		"  gl_FragColor.xyz += v_fragmentAddi.xyz * tmp.w;  \n"
		"}  \n"
		;	

	static const char * shape_vs =
		FLOAT_PRECISION
		"\n"
		"attribute vec4 position;  \n"
		"attribute vec4 color;     \n"
		"\n"
		"varying vec4 v_color;  \n"
		"void main()  \n"
		"{  \n"
		"  gl_Position = position;  \n"
		"  v_color = color / 255.0;  \n"
		"}  \n"
		;

	static const char * shape_fs =
		FLOAT_PRECISION
		"\n"
		"varying vec4 v_color;  \n"
		"void main()  \n"
		"{  \n"
		"  gl_FragColor = v_color;  \n"
		"}  \n"
		;

	_rs_init();

	_program_init(&PROG[PROGRAM_NORMAL], sprite_vs, sprite_fs);
	_program_init(&PROG[PROGRAM_ETC1], sprite_vs, etc1_fs);
	_program_init(&PROG[PROGRAM_SHAPE], shape_vs, shape_fs);

	dtex_shader_program(PROGRAM_NORMAL);

	glGenBuffers(1, &VERTEX_BUFFER);
	glBindBuffer(GL_ARRAY_BUFFER, VERTEX_BUFFER);

	glEnable(GL_BLEND);
}

void 
dtex_shader_unload() {
	if (RS == NULL) {
		return;
	}

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);        
	if (PROG[RS->activeprog].sampler1) {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, 0);     
	}

	int i;
	for (i = 0; i < PROGRAM_TOTAL; i++) {
		glDeleteProgram(PROG[i].prog);
	}

	glDeleteBuffers(1, &VERTEX_BUFFER);
	glDeleteBuffers(1, &INDEX_BUFFER);
	free(RS);
	RS = NULL;
}

static void
_rs_commit() {
	if (RS == NULL || RS->object == 0)
		return;

#ifndef USED_IN_EDITOR
	assert(dtex_gl_is_texture(dtex_gl_get_curr_texrute()));
#endif // USED_IN_EDITOR
	assert(dtex_gl_get_curr_target() != 0);

	glBindBuffer(GL_ARRAY_BUFFER, VERTEX_BUFFER);
	glBufferData(GL_ARRAY_BUFFER, 24 * RS->object * sizeof(float), RS->vb, GL_DYNAMIC_DRAW);

// #ifdef __MACOSX
// 	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, INDEX_BUFFER);
// #endif

	glEnableVertexAttribArray(ATTRIB_VERTEX);

	glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, GL_FALSE, 24, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(ATTRIB_TEXTCOORD);
	glVertexAttribPointer(ATTRIB_TEXTCOORD, 2, GL_FLOAT, GL_FALSE, 24, BUFFER_OFFSET(8));
	glEnableVertexAttribArray(ATTRIB_COLOR);
	glVertexAttribPointer(ATTRIB_COLOR, 4, GL_UNSIGNED_BYTE, GL_FALSE, 24, BUFFER_OFFSET(16));
	glEnableVertexAttribArray(ATTRIB_ADDITIVE);
	glVertexAttribPointer(ATTRIB_ADDITIVE, 4, GL_UNSIGNED_BYTE, GL_FALSE, 24, BUFFER_OFFSET(20));  

	glDrawElements(GL_TRIANGLES, 6 * RS->object, GL_UNSIGNED_SHORT, 0);

	glDisableVertexAttribArray(ATTRIB_VERTEX);
	glDisableVertexAttribArray(ATTRIB_TEXTCOORD);
	glDisableVertexAttribArray(ATTRIB_COLOR);
	glDisableVertexAttribArray(ATTRIB_ADDITIVE);

#ifdef __MACOSX
 	glBindBuffer(GL_ARRAY_BUFFER, 2);
	// glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 1);
#else
	glBindBuffer(GL_ARRAY_BUFFER, 0);
#endif

	RS->object = 0;

	dtex_stat_add_drawcall();
}

void 
dtex_shader_program(int n) {
	if (RS->activeprog == n) {
		return;
	}

	_rs_commit();

	glUseProgram(PROG[n].prog);
	RS->activeprog = n;
	if (n == PROGRAM_NULL) {
		return;
	}

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
	RS->texture = 0;

	if (n == PROGRAM_ETC1) {
		glUniform1i(PROG[n].sampler0, 0);
		glUniform1i(PROG[n].sampler1, 1);
	} else {
		glUniform1i(PROG[n].sampler0, 0);
	}
}

void 
dtex_shader_blend(int mode) {
	if (RS->blend == mode) {
		return;
	}

	_rs_commit();
	RS->blend = mode;
	glBlendFunc(GL_ONE, mode);
}

void 
dtex_shader_texture(int id) {
	if (RS->texture == id) {
		return;
	}

	_rs_commit();
	RS->texture = (GLuint)id;
	glBindTexture(GL_TEXTURE_2D, RS->texture);
}

void 
dtex_shader_set_target(int id) {
	RS->target = id;
}

int
dtex_shader_get_target() {
	return RS->target;
}

static void
_copy_vertex(const float vb[16]) {
	float* ptr = RS->vb + 24 * RS->object;
	memcpy(ptr, vb, 4 * sizeof(float));
	ptr += 4;
	memcpy(ptr, &MULTI_COL, sizeof(int));
	ptr += 1;
	memcpy(ptr, &ADD_COL, sizeof(int));
	ptr += 1;  
	memcpy(ptr, &vb[4], 4 * sizeof(float));
	ptr += 4;
	memcpy(ptr, &MULTI_COL, sizeof(int));
	ptr += 1;
	memcpy(ptr, &ADD_COL, sizeof(int));
	ptr += 1;    
	memcpy(ptr, &vb[8], 4 * sizeof(float));
	ptr += 4;
	memcpy(ptr, &MULTI_COL, sizeof(int));
	ptr += 1;
	memcpy(ptr, &ADD_COL, sizeof(int));
	ptr += 1;    
	memcpy(ptr, &vb[12], 4 * sizeof(float));
	ptr += 4;
	memcpy(ptr, &MULTI_COL, sizeof(int));
	ptr += 1; 
	memcpy(ptr, &ADD_COL, sizeof(int));
}

void 
dtex_shader_draw(const float vb[16]) {
	_copy_vertex(vb);
	if (++RS->object >= MAX_COMMBINE) {
		_rs_commit();
	}
}

void 
dtex_shader_draw_triangle(const float* vb, int count) {
	glBindBuffer(GL_ARRAY_BUFFER, VERTEX_BUFFER);
	glBufferData(GL_ARRAY_BUFFER, count * 3 * sizeof(float), vb, GL_DYNAMIC_DRAW);

	glEnableVertexAttribArray(ATTRIB_VERTEX);
	glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, GL_FALSE, 12, 0);

	glEnableVertexAttribArray(ATTRIB_COLOR);
	glVertexAttribPointer(ATTRIB_COLOR, 4, GL_UNSIGNED_BYTE, GL_FALSE, 12, BUFFER_OFFSET(8));

	glDrawArrays(GL_TRIANGLES, 0, count);

	glDisableVertexAttribArray(ATTRIB_VERTEX);
	glDisableVertexAttribArray(ATTRIB_COLOR);
}

void 
dtex_shader_flush() {
	_rs_commit();
}