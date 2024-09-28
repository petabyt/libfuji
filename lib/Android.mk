LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

CAMLIB := camlib
LUA := $(LOCAL_PATH)/../third_party/lua
LIBUI := $(LOCAL_PATH)/../third_party//libui/lib
LIBJPEG := $(LOCAL_PATH)/../third_party//android-libjpeg-turbo

CAMLIB_CORE := $(addprefix $(CAMLIB)/src/,transport.c data.c enum_dump.c enums.c canon.c operations.c packet.c lib.c conv.c generic.c object.c)
CAMLIB_CORE += $(addprefix $(CAMLIB)/src/,lua/lua-cjson/strbuf.c lua/lua-cjson/lua_cjson.c lua/lua.c lua/runtime.c)

FUDGE_CORE := main.c backend.c fuji.c fuji_usb.c tester.c net.c scripts.c camlib.c usb.c data.c liveview.c discovery.c exif.c uilua.c
FUDGE_CORE += settings.c
LOCAL_MODULE := fudge
LOCAL_CFLAGS := -D ANDROID -Wall -Wshadow -Wcast-qual -Wpedantic -Werror=incompatible-pointer-types -Werror=deprecated-declarations -g
LOCAL_SRC_FILES := $(FUDGE_CORE) $(CAMLIB_CORE)
LOCAL_C_INCLUDES += $(LOCAL_PATH) $(LOCAL_PATH)/$(CAMLIB)/src $(LOCAL_PATH)/$(CAMLIB)/src/lua $(LUA)
LOCAL_LDLIBS += -llog -landroid -lEGL -lGLESv2 -lGLESv1_CM

LOCAL_C_INCLUDES += $(LIBUI)

LOCAL_CFLAGS += -DMEM_SRCDST_SUPPORTED

LOCAL_C_INCLUDES += $(LIBJPEG)/libjpeg-turbo-2.0.1 $(LIBJPEG)/include

LOCAL_STATIC_LIBRARIES += lua libui libjpeg-turbo

include $(BUILD_SHARED_LIBRARY)
# This has to be in the order of LOCAL_STATIC_LIBRARIES.. ugh
include $(LOCAL_PATH)/Lua.mk
include $(LIBJPEG)/Android.mk
include $(LIBUI)/Android.mk