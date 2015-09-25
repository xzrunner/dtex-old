#include "dtex_texture_loader.h"
#include "dtex_math.h"
#include "dtex_gl.h"
#include "dtex_pvr.h"
#include "dtex_etc1.h"
#include "dtex_stream_import.h"
#include "dtex_texture_pool.h"
#include "dtex_typedef.h"

#include "opengl.h"

#include <stdlib.h>
#include <assert.h>

static inline GLuint
_texture_create(struct dtex_import_stream* is, int format, int width, int height) {
	if ((format == TEXTURE8) || (IS_POT(width) && IS_POT(height))) {
		glPixelStorei(GL_UNPACK_ALIGNMENT,4);
	} else {
		glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	}

	const char* data = (char*)is->stream;
	GLuint texid = dtex_prepare_texture(GL_TEXTURE0);
	switch(format) {
	case TEXTURE8:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		dtex_stat_add_texture(texid, width, height);
		break;
	case TEXTURE4:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height, 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, data);
		dtex_stat_add_texture(texid, width, height);
		break;
	default:
		return 0;
	}
	return texid;
}

static inline GLuint
_pvr_texture_create(uint8_t* data, size_t sz, int internal_format, int width, int height) {
	GLuint texid = dtex_prepare_texture(GL_TEXTURE0);
	uint8_t* ptr = data;
	for (int i = 0; ptr - data < sz; ++i) {
		int ori_sz = ptr[0] | ptr[1] << 8 | ptr[2] << 16 | ptr[3] << 24;
#ifdef __APPLE__
		glCompressedTexImage2D(GL_TEXTURE_2D, i, internal_format, width, height, 0, ori_sz, ptr+4);
#else
		uint8_t* uncompressed = dtex_pvr_decode(ptr+4, width, height);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height, 0, GL_RGBA, GL_UNSIGNED_BYTE, uncompressed);		
		free(uncompressed);
		dtex_stat_add_texture(texid, width, height);
#endif
		ptr += 4 + ori_sz;
	}
	return texid;
}

static inline void
_etc1_texture_create(struct dtex_import_stream* is, int width, int height, GLuint* id_rgb, GLuint* id_alpha) {
	size_t sz = (width * height) >> 1;
	*id_rgb = dtex_prepare_texture(GL_TEXTURE0);
	*id_alpha = dtex_prepare_texture(GL_TEXTURE1);
#ifdef __ANDROID__
	glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_ETC1_RGB8_OES, width, height, 0, sz, data);	
	glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_ETC1_RGB8_OES, width, height, 0, sz, data+sz);
#else
	const char* data = (char*)is->stream;

	uint8_t* buf_rgb = dtex_etc1_decode(data, width, height);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf_rgb);
	free(buf_rgb);
	dtex_stat_add_texture(*id_rgb, width, height);

	uint8_t* buf_alpha = dtex_etc1_decode(data + sz, width, height);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf_alpha);
	free(buf_alpha);
	dtex_stat_add_texture(*id_alpha, width, height);
#endif
}

// void 
// dtex_load_pvr(struct dtex_import_stream* is, struct dtex_raw_tex* tex) {
// 	assert(tex->width == (buf[1] | buf[2] << 8)
// 		&& tex->height == (buf[3] | buf[4] << 8));
// 	int internal_format = 0;
// #ifdef __APPLE__ 
// 	int format = buf[0];
// 	if (format == 4) {
// 		internal_format = GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
// 	} else if (format == 2) {
// 		internal_format = GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
// 	} else {
// 		assert(0);
// 	}
// #endif
// 	tex->id = _pvr_texture_create(buf+5, sz-5, internal_format, tex->width, tex->height);
// }

void 
dtex_load_texture_desc(struct dtex_import_stream* is, struct dtex_raw_tex* tex) {
	tex->format = dtex_import_uint8(is);

	tex->width = dtex_import_uint16(is);
	tex->height = dtex_import_uint16(is);

	tex->id = tex->id_alpha = 0;
}

void 
dtex_load_texture_all(struct dtex_import_stream* is, struct dtex_raw_tex* tex) {
	tex->format = dtex_import_uint8(is);
	
	tex->width = dtex_import_uint16(is);
	tex->height = dtex_import_uint16(is);

	switch (tex->format) {
	case TEXTURE4: case TEXTURE8:
		tex->id = _texture_create(is, tex->format, tex->width, tex->height);
		break;
	case PVRTC:
		// todo
//		tex->id = _pvr_texture_create(buf+5, sz-5, internal_format, tex->width, tex->height);
		break;
	case PKMC:
		_etc1_texture_create(is, tex->width, tex->height, &tex->id, &tex->id_alpha);
		break;
	}
}