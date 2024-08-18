LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

CAMLIB := camlib
LUA := lua
LIBUI := $(LOCAL_PATH)/libui/lib
LIBJPEG := $(LOCAL_PATH)/android-libjpeg-turbo

CAMLIB_CORE := $(addprefix $(CAMLIB)/src/,transport.c data.c enum_dump.c enums.c canon.c operations.c packet.c lib.c conv.c generic.c object.c)
CAMLIB_CORE += $(CAMLIB)/lua/lua-cjson/strbuf.c $(CAMLIB)/lua/lua-cjson/lua_cjson.c $(CAMLIB)/lua/lua.c $(CAMLIB)/lua/runtime.c

FUDGE_CORE := main.c backend.c fuji.c fuji_usb.c tester.c net.c scripts.c camlib.c usb.c data.c liveview.c discovery.c exif.c uilua.c
FUDGE_CORE += settings.c

LUA_CORE := $(addprefix $(LUA)/,lbaselib.c lauxlib.c lapi.c lcode.c lctype.c ldebug.c ldo.c ldump.c lfunc.c lgc.c llex.c lmem.c lobject.c lopcodes.c lparser.c lstate.c lstring.c ltable.c ltm.c lundump.c lvm.c lzio.c ltests.c)

LOCAL_MODULE := fudge
LOCAL_CFLAGS := -D ANDROID -Wall -Wshadow -Wcast-qual -Wpedantic -Werror=incompatible-pointer-types -Werror=deprecated-declarations -g
LOCAL_SRC_FILES := $(FUDGE_CORE) $(CAMLIB_CORE) $(LUA_CORE)
LOCAL_C_INCLUDES += $(LOCAL_PATH) $(LOCAL_PATH)/$(CAMLIB)/src $(LOCAL_PATH)/$(CAMLIB)/lua $(LOCAL_PATH)/$(LUA)
LOCAL_LDLIBS += -llog -landroid -lEGL -lGLESv2 -lGLESv1_CM

LOCAL_C_INCLUDES += $(LIBUI)

LOCAL_CFLAGS += -DMEM_SRCDST_SUPPORTED

LOCAL_C_INCLUDES += $(LIBJPEG)/libjpeg-turbo-2.0.1 $(LIBJPEG)/include

LOCAL_STATIC_LIBRARIES += libui libjpeg-turbo

include $(BUILD_SHARED_LIBRARY)

include $(LIBJPEG)/Android.mk
include $(LIBUI)/Android.mk