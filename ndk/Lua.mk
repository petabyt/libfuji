LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LUA := $(LOCAL_PATH)/../third_party/lua
LOCAL_MODULE := liblua
LOCAL_CFLAGS := -O2
LOCAL_SRC_FILES := $(addprefix $(LUA)/,lbaselib.c lauxlib.c lapi.c lcode.c lctype.c ldebug.c ldo.c ldump.c lfunc.c lgc.c llex.c lmem.c lobject.c lopcodes.c lparser.c lstate.c lstring.c ltable.c ltm.c lundump.c lvm.c lzio.c ltests.c)
LOCAL_C_INCLUDES += $(LUA)

include $(BUILD_STATIC_LIBRARY)