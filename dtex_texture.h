#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_texture_h
#define dynamic_texture_texture_h

struct dtex_texture {
	unsigned int tex;
	int width, height;
	// todo inv w & h

	struct dtex_raw_tex* raw_tex;

	struct dtex_packer* packer;		
};

struct dtex_buffer;

struct dtex_texture* dtex_new_tex(struct dtex_buffer*);
struct dtex_texture* dtex_new_tex_with_packer(struct dtex_buffer*, int packer_cap);
// todo return buf
void dtex_del_tex(struct dtex_buffer*, struct dtex_texture*);

void dtex_clear_tex(struct dtex_texture*, struct dtex_buffer*);

#endif // dynamic_texture_texture_h

#ifdef __cplusplus
}
#endif