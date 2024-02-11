LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

CAMLIB_CORE := $(addprefix camlib/src/,transport.c bind.c data.c enum_dump.c enums.c canon.c liveview.c operations.c packet.c lib.c ml.c conv.c generic.c)
CAMLIB_CORE += camlib/lua/lua-cjson/strbuf.c camlib/lua/lua-cjson/lua_cjson.c camlib/lua/lua.c camlib/lua/runtime.c

FUDGE_CORE := main.c jni.c fuji.c tester.c models.c net.c viewer.c
FUDGE_CORE += scripts.c camlib.c usb.c

LUA_CORE := $(addprefix lua/,lbaselib.c lauxlib.c lapi.c lcode.c lctype.c ldebug.c ldo.c ldump.c lfunc.c lgc.c llex.c lmem.c lobject.c lopcodes.c lparser.c lstate.c lstring.c ltable.c ltm.c lundump.c lvm.c lzio.c ltests.c)

LOCAL_MODULE := fujiapp
LOCAL_CFLAGS := -Wall -D ANDROID
LOCAL_SRC_FILES := $(FUDGE_CORE) $(CAMLIB_CORE) $(LUA_CORE)
LOCAL_C_INCLUDES += $(LOCAL_PATH) $(LOCAL_PATH)/camlib/src $(LOCAL_PATH)/camlib/lua $(LOCAL_PATH)/lua
LOCAL_LDLIBS += -llog

LIBUIFW := libuifw

LOCAL_SRC_FILES += $(LIBUIFW)/libui.c $(LIBUIFW)/lib.c $(LIBUIFW)/lua.c
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(LIBUIFW)

include $(BUILD_SHARED_LIBRARY)
