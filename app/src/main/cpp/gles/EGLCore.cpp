//
// Created by zzh on 2018/5/31 0031.
//

#include "EGLCore.h"

#include <android/log.h>
#include <android/native_window.h>
#include <EGL/eglext.h>

#define LOG_TAG "EGLCore"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

EGLCore::EGLCore() : mDisplay(EGL_NO_DISPLAY), mSurface(EGL_NO_SURFACE),
                     mContext(EGL_NO_CONTEXT) {

}

EGLCore::~EGLCore() {
    mDisplay = EGL_NO_DISPLAY;
    mSurface = EGL_NO_SURFACE;
    mContext = EGL_NO_CONTEXT;
}

GLboolean EGLCore::buildContext(ANativeWindow *window) {
    // 与本地窗口系统通信
    mDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (mDisplay == EGL_NO_DISPLAY) {
        LOGE("eglGetDisplay failed: %d", eglGetError());
        return GL_FALSE;
    }

    GLint majorVersion;
    GLint minorVersion;
    if (!eglInitialize(mDisplay, &majorVersion, &minorVersion)) {
        LOGE("eglInitialize failed: %d", eglGetError());
        return GL_FALSE;
    }

    EGLConfig config;
    // 查找可用的 surface 配置
    EGLint numConfigs = 0;
    EGLint attribList[] = {
            EGL_RED_SIZE, 5,
            EGL_GREEN_SIZE, 6,
            EGL_BLUE_SIZE, 5,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_NONE
    };

    // 让 EGL 推荐匹配的 EGLConfig
    if (!eglChooseConfig(mDisplay, attribList, &config, 1, &numConfigs)) {
        LOGE("eglChooseConfig failed: %d", eglGetError());
        return GL_FALSE;
    }

    if (numConfigs < 1) {
        LOGE("eglChooseConfig get configs number less than one");
        return GL_FALSE;
    }

    // 创建渲染上下文(rendering context)
    GLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    mContext = eglCreateContext(mDisplay, config, EGL_NO_CONTEXT, contextAttribs);
    if (mContext == EGL_NO_CONTEXT) {
        LOGE("eglCreateContext failed: %d", eglGetError());
        return GL_FALSE;
    }

    EGLint format = 0;
    if (!eglGetConfigAttrib(mDisplay, config, EGL_NATIVE_VISUAL_ID, &format)) {
        LOGE("eglGetConfigAttrib failed: %d", eglGetError());
        return GL_FALSE;
    }
    ANativeWindow_setBuffersGeometry(window, 0, 0, format);

    // 创建 On-Screen 渲染区域
    mSurface = eglCreateWindowSurface(mDisplay, config, window, 0);
    if (mSurface == EGL_NO_SURFACE) {
        LOGE("eglCreateWindowSurface failed: %d", eglGetError());
        return GL_FALSE;
    }

    // 把 EGLContext 和 EGLSurface 关联起来
    if (!eglMakeCurrent(mDisplay, mSurface, mSurface, mContext)) {
        LOGE("eglMakeCurrent failed: %d", eglGetError());
        return GL_FALSE;
    }

    LOGD("buildContext succeed");
    return GL_TRUE;
}

void EGLCore::swapBuffer() {
    eglSwapBuffers(mDisplay, mSurface);
}

void EGLCore::release() {
    eglDestroySurface(mDisplay, mSurface);
    eglMakeCurrent(mDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(mDisplay, mContext);
}