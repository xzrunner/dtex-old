#ifndef dynamic_texture_fbo_h
#define dynamic_texture_fbo_h

struct dtex_fbo;
struct dtex_texture;

struct dtex_fbo* dtex_new_fbo();
void dtex_del_fbo(struct dtex_fbo*);

void dtex_shader_fbo(struct dtex_fbo*);
void dtex_fbo_bind_texture(struct dtex_fbo*, struct dtex_texture*);

void dtex_fbo_bind(struct dtex_fbo*);
void dtex_fbo_unbind();

#endif // dynamic_texture_fbo_h