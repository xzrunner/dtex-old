#include "dtex_gl.h"
#include "dtex_shader.h"

#include <opengl.h>

GLuint 
dtex_prepare_texture(int texture) {
	GLuint texid = 0;

	glActiveTexture(texture);
	glGenTextures(1, &texid);

	dtex_shader_texture(texid);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	return texid;	
}