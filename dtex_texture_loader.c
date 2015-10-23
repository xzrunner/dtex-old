#include "dtex_texture_loader.h"
#include "dtex_gl.h"
#include "dtex_pvr.h"
#include "dtex_etc1.h"
#include "dtex_stream_import.h"
#include "dtex_typedef.h"
#include "dtex_statistics.h"
#include "dtex_texture.h"
#include "dtex_render.h"
#include "dtex_package.h"
#include "dtex_log.h"

#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <string.h>

// static inline GLuint
// _pvr_texture_create(uint8_t* data, size_t sz, int internal_format, int width, int height) {
// 	GLuint texid = dtex_prepare_texture(GL_TEXTURE0);
// 	uint8_t* ptr = data;
// 	for (int i = 0; ptr - data < sz; ++i) {
// 		int ori_sz = ptr[0] | ptr[1] << 8 | ptr[2] << 16 | ptr[3] << 24;
// #ifdef __APPLE__
// 		glCompressedTexImage2D(GL_TEXTURE_2D, i, internal_format, width, height, 0, ori_sz, ptr+4);
// #else
// 		uint8_t* uncompressed = dtex_pvr_decode(ptr+4, width, height);
// 		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height, 0, GL_RGBA, GL_UNSIGNED_BYTE, uncompressed);		
// 		free(uncompressed);
// 		dtex_stat_add_texture(texid, width, height);
// #endif
// 		ptr += 4 + ori_sz;
// 	}
// 	return texid;
// }

// void 
// dtex_load_pvr(struct dtex_import_stream* is, struct dtex_texture* tex) {
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

int 
dtex_load_all_textures_desc(struct dtex_import_stream* is, struct dtex_package* pkg, float scale) {
	int sz = dtex_import_uint32(is);
	pkg->texture_count = sz;
	for (int i = 0; i < sz; ++i) {
		int edge = dtex_import_uint32(is);

		struct dtex_texture* tex = dtex_texture_create_raw(pkg->LOD);
		if (!tex) {
			dtex_fault("_unpack_memory_to_preload_all_textures dtex_texture_create_raw err.");
		}

		tex->t.RAW.format = TEXTURE8;	// todo

		tex->width = floor(edge * tex->t.RAW.lod_scale + 0.5f);
		tex->height = floor(edge * tex->t.RAW.lod_scale + 0.5f);
		tex->inv_width = 1.0f / tex->width;
		tex->inv_height = 1.0f / tex->height;

		tex->t.RAW.scale = scale;

		tex->id = 0;
		tex->t.RAW.id_alpha = 0;

		assert(!pkg->textures[i]);
		pkg->textures[i] = tex;
	}
	return sz;
}

void 
dtex_load_texture_only_desc(struct dtex_import_stream* is, struct dtex_texture* tex, float scale) {
	assert(tex->type == DTEX_TT_RAW);

	tex->t.RAW.format = dtex_import_uint8(is);

	float w = dtex_import_uint16(is) * scale;
	float h = dtex_import_uint16(is) * scale;
	tex->width = floor(w + 0.5f);
	tex->height = floor(h + 0.5f);
	tex->inv_width = 1.0f / tex->width;
	tex->inv_height = 1.0f / tex->height;

	tex->t.RAW.scale = scale;

	tex->id = 0;
	tex->t.RAW.id_alpha = 0;
}

void
_scale_texture(struct dtex_texture* tex, float scale, bool create_by_ej) {
	assert(tex->type == DTEX_TT_RAW);

	uint16_t new_w = floor(tex->width * scale + 0.5f);
	uint16_t new_h = floor(tex->height * scale + 0.5f);
	
	uint8_t* empty_data = (uint8_t*)malloc(new_w*new_h*4);
	memset(empty_data, 0, new_w*new_h*4);
	unsigned int gl_id;
	int uid_3rd;
	dtex_gl_create_texture(DTEX_TF_RGBA8, new_w, new_h, empty_data, 0, &gl_id, &uid_3rd, create_by_ej);
	free(empty_data);

	float vb[16];
	vb[0]  = -1, vb[1]  = -1;
	vb[2]  =  0, vb[3]  =  0;
	vb[4]  = -1, vb[5]  =  1;
	vb[6]  =  0, vb[7]  =  1;
	vb[8]  =  1, vb[9]  =  1;
	vb[10] =  1, vb[11] =  1;
	vb[12] =  1, vb[13] = -1;
	vb[14] =  1, vb[15] =  0;

	struct dtex_texture dst;
	dst.id = gl_id;
	dst.uid_3rd = uid_3rd;
	dst.width = new_w;
	dst.height = new_h;
	dst.inv_width = 1.0f / new_w;
	dst.inv_height = 1.0f / new_h;
	dst.uid = -1;
	dst.type = DTEX_TT_MID;
	dst.t.MID.tp = NULL;

	dtex_draw_to_texture(tex, &dst, vb);
	dtex_draw_finish();

	dtex_gl_release_texture(tex->id, 0);

	tex->t.RAW.format = TEXTURE8;
	tex->id = gl_id;
	tex->uid_3rd = uid_3rd;
	tex->t.RAW.id_alpha = 0;
	tex->width = new_w;
	tex->height = new_h;
	tex->inv_width = 1.0f / new_w;
	tex->inv_height = 1.0f / new_h;
}

void 
dtex_load_texture_all(struct dtex_import_stream* is, struct dtex_texture* tex, bool create_by_ej) {
	assert(tex->type == DTEX_TT_RAW);

	int format = dtex_import_uint8(is);
	int width = dtex_import_uint16(is),
		height = dtex_import_uint16(is);
// 	assert(tex->t.RAW.format == format 
// 		&& tex->width == floor(width * tex->t.RAW.scale + 0.5f)
// 		&& tex->height == floor(height * tex->t.RAW.scale + 0.5f));
	tex->t.RAW.format = format;
	tex->width = width;
	tex->height = height;
	tex->inv_width = 1.0f / width;
	tex->inv_height = 1.0f / height;

	switch (tex->t.RAW.format) {
	case TEXTURE4: 
		dtex_gl_create_texture(DTEX_TF_RGBA4, width, height, is->stream, 0, &tex->id, &tex->uid_3rd, create_by_ej);
		break;
	case TEXTURE8:
		dtex_gl_create_texture(DTEX_TF_RGBA8, width, height, is->stream, 0, &tex->id, &tex->uid_3rd, create_by_ej);
		break;
	case PVRTC:
		dtex_gl_create_texture(DTEX_TF_PVR4, width, height, is->stream, 0, &tex->id, &tex->uid_3rd, create_by_ej);
		// todo
//		tex->id = _pvr_texture_create(buf+5, sz-5, internal_format, width, height);
		break;
	case PKMC:
		dtex_gl_create_texture(DTEX_TF_ETC1, width, height, is->stream, 0, &tex->id, &tex->uid_3rd, create_by_ej);
		// fixme
		dtex_gl_create_texture(DTEX_TF_ETC1, width, height, is->stream + ((width * height) >> 1), 1, &tex->t.RAW.id_alpha, &tex->uid_3rd, create_by_ej);
		break;
	}

	if (tex->t.RAW.scale != 1 && (tex->t.RAW.format == TEXTURE4 || tex->t.RAW.format == TEXTURE8)) {
		_scale_texture(tex, tex->t.RAW.scale, create_by_ej);
	}
}