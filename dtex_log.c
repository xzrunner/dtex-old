#include "dtex_log.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __ANDROID__
    #include <android/log.h>
    #include <jni.h>
    #define  LOG_TAG                    "dtex"
    #define  pf_log(...)                __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
    #define  pf_vprint(format, ap)      __android_log_vprint(ANDROID_LOG_DEBUG, LOG_TAG, (format), (ap))
#else
    #define pf_log                      printf
    #define pf_vprint                   vprintf
#endif

//#define DEBUG_FILE

#ifdef DEBUG_FILE
static const char* DEBUG_FILENAME = "lr-debug.txt";
#endif // DEBUG_FILE

void 
dtex_log_init() {
#ifdef DEBUG_FILE
	FILE* fp = fopen(DEBUG_FILENAME, "w");
    if (fp) {
        fprintf(fp, "\n");
    }
	fclose(fp);
#endif // DEBUG_FILE
}

void 
dtex_fault(const char* format, ...) {
	if (format[0] == '!') {
		va_list ap;
		va_start(ap, format);
		pf_vprint(format+1, ap);
		va_end(ap);
	} else {
		va_list ap;
		va_start(ap, format);
		pf_vprint(format, ap);
		va_end(ap);
		exit(1);
	}
}

void 
dtex_info(const char* format, ...) {
	pf_log("[DTEX] info: ");
	va_list ap;
	va_start(ap, format);
	pf_vprint(format+1, ap);
	va_end(ap);
	pf_log("\n");
}

void 
dtex_warning(const char* format, ...) {
	pf_log("[DTEX] warning: ");
	va_list ap;
	va_start(ap, format);
	pf_vprint(format+1, ap);
	va_end(ap);	
	pf_log("\n");
}

void 
dtex_debug(const char* format, ...) {
// 	pf_log("++++++++++++++ [DTEX] debug: ");
// 	va_list ap;
// 	va_start(ap, format);
// 	pf_vprint(format+1, ap);
// 	va_end(ap);	
// 	pf_log("\n");
}

void 
dtex_debug_to_file(const char* format, ...) {
#ifdef DEBUG_FILE
	FILE* fp = fopen(DEBUG_FILENAME, "a");
	if (fp) {
		va_list ap;
		va_start(ap, format);
		vfprintf(fp, format+1, ap);
		va_end(ap);
	    
		fprintf(fp, "\n");
	}
	fclose(fp);
#endif // DEBUG_FILE
}