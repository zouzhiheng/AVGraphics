//
// Created by Administrator on 2018/3/2 0002.
//

#ifndef VIDEOEDITOR_LOG_H
#define VIDEOEDITOR_LOG_H

#include <jni.h>
#include <android/log.h>

#define LOG_TAG "AVGraphics_JNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#endif //VIDEOEDITOR_LOG_H
