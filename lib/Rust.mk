LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
RUST_TARGET := aarch64-linux-android
else ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
RUST_TARGET := armv7-linux-androideabi
else ifeq ($(TARGET_ARCH_ABI),x86)
RUST_TARGET := i686-linux-android
else ifeq ($(TARGET_ARCH_ABI),x86_64)
RUST_TARGET := x86_64-linux-android
endif
RUST=../rust

LOCAL_MODULE := rust
LOCAL_SRC_FILES := $(shell realpath $(LOCAL_PATH)/$(RUST)/target/$(RUST_TARGET)/release/*.a)

include $(PREBUILT_STATIC_LIBRARY)

