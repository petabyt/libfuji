LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

CAMLIB := ../camlib

CAMLIB_CORE := $(addprefix $(CAMLIB)/src/,transport.c data.c enum_dump.c enums.c canon.c operations.c packet.c lib.c conv.c generic.c)
CAMLIB_CORE += $(CAMLIB)/lua/lua-cjson/strbuf.c $(CAMLIB)/lua/lua-cjson/lua_cjson.c $(CAMLIB)/lua/lua.c $(CAMLIB)/lua/runtime.c

FUDGE_CORE := main.c jni.c fuji.c fuji_usb.c tester.c net.c viewer.c scripts.c camlib.c usb.c progress.c data.c liveview.c discovery.c exif.c

LUA := ../lua/

LUA_CORE := $(addprefix $(LUA),lbaselib.c lauxlib.c lapi.c lcode.c lctype.c ldebug.c ldo.c ldump.c lfunc.c lgc.c llex.c lmem.c lobject.c lopcodes.c lparser.c lstate.c lstring.c ltable.c ltm.c lundump.c lvm.c lzio.c ltests.c)

LOCAL_MODULE := fujiapp
LOCAL_CFLAGS := -D ANDROID -Wall -Wshadow -Wcast-qual -Wpedantic -Werror=incompatible-pointer-types -Werror=deprecated-declarations
LOCAL_SRC_FILES := $(FUDGE_CORE) $(CAMLIB_CORE) $(LUA_CORE)
LOCAL_C_INCLUDES += $(LOCAL_PATH) $(LOCAL_PATH)/$(CAMLIB)/src $(LOCAL_PATH)/$(CAMLIB)/lua $(LOCAL_PATH)/$(LUA)
LOCAL_LDLIBS += -llog -landroid -lEGL -lGLESv2 -lGLESv1_CM

LIBUIFW := libuifw

LOCAL_SRC_FILES += $(LIBUIFW)/libui.c $(LIBUIFW)/lib.c $(LIBUIFW)/lua.c
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(LIBUIFW)

LIBJPEG := $(LOCAL_PATH)/../libjpeg-turbo
LOCAL_CFLAGS += -DMEM_SRCDST_SUPPORTED

LOCAL_C_INCLUDES += $(LIBJPEG)/libjpeg-turbo-2.0.1 $(LIBJPEG)/include

LOCAL_STATIC_LIBRARIES += libjpeg-turbo

include $(BUILD_SHARED_LIBRARY)

include $(LIBJPEG)/Android.mk