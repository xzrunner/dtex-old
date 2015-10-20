#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_debug_h
#define dynamic_texture_debug_h

void dtex_debug_draw(unsigned int texid);
void dtex_debug_draw_with_pos(unsigned int texid, float xmin, float ymin, float xmax, float ymax);

#ifndef USED_IN_EDITOR
void dtex_debug_draw_ej(int uid_3rd, int pos);
#endif // USED_IN_EDITOR

#endif // dynamic_texture_debug_h

#ifdef __cplusplus
}
#endif