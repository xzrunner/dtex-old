#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_hardware_resources_h
#define dynamic_hardware_resources_h

#include <stdbool.h>

void dtex_hard_res_init(int need_texture_area);

int dtex_max_texture_size();

// bool dtex_hard_res_fetch_texture(int edge);
// void dtex_hard_res_return_texture(int edge);

#endif // dynamic_hardware_resources_h

#ifdef __cplusplus
}
#endif