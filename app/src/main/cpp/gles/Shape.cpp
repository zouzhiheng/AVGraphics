//
// Created by Administrator on 2018/5/30 0030.
//

#include <jni.h>
#include <android/log.h>
#include <pthread.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include "Shape.h"
#include "Triangle.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#define LOG_TAG "Shape"
#define LOGI(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

void *threadStartCallback(void *arg);

Shape::Shape(ANativeWindow *window) : mWindow(window), mEGLCore(new EGLCore()), mThreadId(0),
                                      mIsRendering(false) {
    pthread_mutex_init(&mMutex, nullptr);
    pthread_cond_init(&mCondition, nullptr);
}

void Shape::resize(int width, int height) {
    mWidth = width;
    mHeight = height;
}

void Shape::start() {
    if (mWindow == nullptr || mWidth == 0 || mHeight == 0) {
        LOGE("not configured, cannot start");
        return;
    }
    pthread_create(&mThreadId, nullptr, threadStartCallback, (void *) this);
}

void *threadStartCallback(void *arg) {
    Shape *shape = (Shape *) arg;
    if (!shape->init()) {
        return 0;
    }
    shape->renderLoop();
    shape->release();
    return 0;
}

bool Shape::init() {
    if (!mEGLCore->buildContext(mWindow)) {
        LOGE("buildContext failed");
        return false;
    }

    return true;
}

void Shape::renderLoop() {
    mIsRendering = true;
    LOGI("renderLoop started");
    while (mIsRendering) {
        pthread_mutex_lock(&mMutex);

        doDraw();
        pthread_cond_wait(&mCondition, &mMutex);

        pthread_mutex_unlock(&mMutex);
    }
    LOGI("renderLoop ended");
}

void Shape::draw() {
    pthread_mutex_lock(&mMutex);
    mIsRendering = true;
    pthread_cond_signal(&mCondition);
    pthread_mutex_unlock(&mMutex);
}

void Shape::stop() {
    pthread_mutex_lock(&mMutex);
    mIsRendering = false;
    pthread_cond_signal(&mCondition);
    pthread_mutex_unlock(&mMutex);

    pthread_join(mThreadId, 0);
}

void Shape::release() {
    pthread_mutex_destroy(&mMutex);
    pthread_cond_destroy(&mCondition);

    glDeleteProgram(mProgram);
    mEGLCore->release();

    if (mWindow) {
        ANativeWindow_release(mWindow);
        mWindow = nullptr;
    }

    if (mEGLCore) {
        delete mEGLCore;
        mEGLCore = nullptr;
    }
}

Triangle *triangle = nullptr;

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_gles_ShapeActivity__1init(JNIEnv *env, jclass type,
                                                              jobject surface, jint width,
                                                              jint height) {
    if (triangle) {
        delete triangle;
    }
    ANativeWindow *window = ANativeWindow_fromSurface(env, surface);
    triangle = new Triangle(window);
    triangle->resize(width, height);
    triangle->start();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_gles_ShapeActivity__1draw(JNIEnv *env, jclass type) {
    if (triangle == nullptr) {
        LOGE("draw error, shape is null");
        return;
    }
    triangle->draw();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_gles_ShapeActivity__1release(JNIEnv *env, jclass type) {
    if (triangle) {
        triangle->stop();
        delete triangle;
        triangle = nullptr;
    }
}
#pragma clang diagnostic pop