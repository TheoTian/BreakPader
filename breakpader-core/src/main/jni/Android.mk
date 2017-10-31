ROOT_PATH := $(call my-dir)
include $(ROOT_PATH)/breakpad/android/google_breakpad/Android.mk

LOCAL_PATH := $(ROOT_PATH)
include $(CLEAR_VARS)

LOCAL_MODULE    := breakpader
LOCAL_SRC_FILES := native_breakpad.cpp
LOCAL_LDLIBS := -llog
LOCAL_STATIC_LIBRARIES += breakpad_client breakpad_tools breakpad_processor

include $(BUILD_SHARED_LIBRARY)
