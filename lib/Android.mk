LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

CAMLIB := camlib

CAMLIB_CORE := $(addprefix $(CAMLIB)/src/,transport.c bind.c data.c enum_dump.c enums.c canon.c liveview.c operations.c packet.c lib.c ml.c conv.c generic.c)
CAMLIB_CORE += $(CAMLIB)/lua/lua-cjson/strbuf.c $(CAMLIB)/lua/lua-cjson/lua_cjson.c $(CAMLIB)/lua/lua.c $(CAMLIB)/lua/runtime.c

FUDGE_CORE := main.c jni.c fuji.c fuji_usb.c tester.c net.c viewer.c
FUDGE_CORE += scripts.c camlib.c usb.c progress.c data.c

LUA := lua/

LUA_CORE := $(addprefix $(LUA),lbaselib.c lauxlib.c lapi.c lcode.c lctype.c ldebug.c ldo.c ldump.c lfunc.c lgc.c llex.c lmem.c lobject.c lopcodes.c lparser.c lstate.c lstring.c ltable.c ltm.c lundump.c lvm.c lzio.c ltests.c)

LOCAL_MODULE := fujiapp
LOCAL_CFLAGS := -D ANDROID -Wall -Wshadow -Wcast-qual -Wpedantic -Werror=incompatible-pointer-types -Werror=deprecated-declarations
LOCAL_SRC_FILES := $(FUDGE_CORE) $(CAMLIB_CORE) $(LUA_CORE)
LOCAL_C_INCLUDES += $(LOCAL_PATH) $(LOCAL_PATH)/$(CAMLIB)/src $(LOCAL_PATH)/$(CAMLIB)/lua $(LOCAL_PATH)/$(LUA)
LOCAL_LDLIBS += -llog

LIBUIFW := libuifw

LOCAL_SRC_FILES += $(LIBUIFW)/libui.c $(LIBUIFW)/lib.c $(LIBUIFW)/lua.c
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(LIBUIFW)

include $(BUILD_SHARED_LIBRARY)
