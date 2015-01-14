#ifndef dynamic_texture_texture_h
#define dynamic_texture_texture_h

typedef unsigned int GLuint;

struct dtex_texture {
	GLuint tex;
	int width, height;
	// todo inv w & h

	struct dtex_packer* packer;		
};

struct dtex_buffer;

struct dtex_texture* dtex_new_tex(struct dtex_buffer*);
struct dtex_texture* dtex_new_tex_with_packer(struct dtex_buffer*, int packer_cap);
// todo return buf
void dtex_del_tex(struct dtex_buffer*, struct dtex_texture*);

void dtex_clear_tex(struct dtex_texture*, struct dtex_buffer*);

#endif // dynamic_texture_texture_h