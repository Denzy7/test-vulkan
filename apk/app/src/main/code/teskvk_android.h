#ifndef MAIN_H
#define MAIN_H
#include <android/native_app_glue/android_native_app_glue.h>
#include <android/log.h>
#define TAG "TestVk"
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__))
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__))
#define arysz(ary) sizeof(ary) / sizeof(ary[0])
struct android_app* testvk_getapp();
#endif
