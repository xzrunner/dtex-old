//#ifdef __cplusplus
//extern "C"
//{
//#endif
//
//#ifndef dynamic_texture_cache1_h
//#define dynamic_texture_cache1_h
//
//#include <stdbool.h>
//
//struct dtex_c1;
//struct dtex_buffer;
//struct draw_params;
//
//struct ej_sprite_pack;
//struct animation;
//
//struct dtex_c1* dtex_c1_create(struct dtex_buffer*);
//void dtex_c1_release(struct dtex_c1*, struct dtex_buffer*);
//
//void dtex_c1_load_anim(struct dtex_c1*, struct ej_sprite_pack* pkg, struct animation* ani, int action);
//bool dtex_c1_draw_anim(struct dtex_c1*, struct ej_sprite_pack* pkg, struct animation* ani, int action, int frame, struct draw_params* params);
//
//void dtex_c1_debug_draw(struct dtex_c1*);
//
//#endif // dynamic_texture_cache1_h
//
//#ifdef __cplusplus
//}
//#endif