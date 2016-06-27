INNER_SAVED_LOCAL_PATH := $(LOCAL_PATH)

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := dtex

LOCAL_CFLAGS := -std=gnu99

LOCAL_C_INCLUDES  := \
	${FS_SRC_PATH} \
	${EJOY2D_SRC_PATH} \
	${PS_SRC_PATH} \
	${LUA_SRC_PATH} \
	${DS_SRC_PATH} \
	${CJSON_SRC_PATH} \
	${LZMA_SRC_PATH} \
	${INNER_SAVED_LOCAL_PATH} \
	${LOGGER_SRC_PATH} \
	${RG_ETC1_SRC_PATH} \
	${ETCPACK_SRC_PATH} \

LOCAL_SRC_FILES := \
	$(subst $(LOCAL_PATH)/,,$(shell find $(LOCAL_PATH) -name "*.c" -print)) \

LOCAL_STATIC_LIBRARIES := \
	rg_etc1 \
	etcpack \

include $(BUILD_STATIC_LIBRARY)	

LOCAL_PATH := $(INNER_SAVED_LOCAL_PATH)