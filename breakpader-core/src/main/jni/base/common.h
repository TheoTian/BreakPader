#ifndef __COMMON_H__
#define __COMMON_H__

#include <jni.h>
#include <android/log.h>

#define RES_OK 0
#define RES_ERROR -1

#define IN
#define OUT

#ifndef DEBUG
#define DEBUG 1
#endif

#ifdef DEBUG
#define TAG "breakpader"
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#else
#define LOGV(...) {}
#define LOGD(...) {}
#define LOGI(...) {}
#define LOGW(...) {}
#define LOGE(...) {}
#endif
#endif /* __COMMON_H__ */
