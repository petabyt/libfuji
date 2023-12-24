LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

CAMLIB_CORE := $(addprefix camlib/src/,transport.c bind.c data.c enum_dump.c enums.c canon.c liveview.c no_usb.c operations.c packet.c lib.c ml.c conv.c generic.c)

FUDGE_CORE := main.c jni.c lib.c fuji.c tester.c models.c net.c

LOCAL_MODULE := fujiapp
LOCAL_CFLAGS := -Wall
LOCAL_SRC_FILES := $(FUDGE_CORE) $(CAMLIB_CORE)
LOCAL_C_INCLUDES += $(LOCAL_PATH) $(LOCAL_PATH)/camlib/src
LOCAL_LDLIBS += -llog

include $(BUILD_SHARED_LIBRARY)
