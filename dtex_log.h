#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_log_h
#define dynamic_texture_log_h

void dtex_log_init();

void dtex_fault(const char* format, ...);

void dtex_info(const char* format, ...);

void dtex_warning(const char* format, ...);

//void dtex_debug(const char* format, ...);

void dtex_debug_to_file(const char* format, ...);

#define dtex_debug dtex_debug_to_file

#endif // dynamic_texture_log_h

#ifdef __cplusplus
}
#endif
