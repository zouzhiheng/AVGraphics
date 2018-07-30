//
// Created by zzh on 2018/5/30 0030.
//

#include <android/log.h>
#include <pthread.h>
#include <android/native_window.h>
#include <log.h>
#include "EGLDemo.h"
#include "Triangle.h"

void *startThreadCallback(void *arg);

EGLDemo::EGLDemo(ANativeWindow *window) : mWindow(window), mEGLCore(new EGLCore()), mStartThread(0),
                                          mIsRendering(false) {
    pthread_mutex_init(&mMutex, nullptr);
    pthread_cond_init(&mCondition, nullptr);
}

void EGLDemo::start() {
    if (mWindow == nullptr || mWidth == 0 || mHeight == 0) {
        LOGE("not configured, cannot start");
        return;
    }
    pthread_create(&mStartThread, nullptr, startThreadCallback, (void *) this);
}

void *startThreadCallback(void *arg) {
    EGLDemo *shape = (EGLDemo *) arg;
    if (shape->doInit()) {
        shape->renderLoop();
        shape->doStop();
    }
    return 0;
}

bool EGLDemo::doInit() {
    if (!mEGLCore->buildContext(mWindow)) {
        LOGE("buildContext failed");
        return false;
    }

    return true;
}

void EGLDemo::renderLoop() {
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

void EGLDemo::doStop() {
    glDeleteProgram(mProgram);
    mEGLCore->release();
}

void EGLDemo::draw() {
    pthread_mutex_lock(&mMutex);
    mIsRendering = true;
    pthread_cond_signal(&mCondition);
    pthread_mutex_unlock(&mMutex);
}

void EGLDemo::stop() {
    pthread_mutex_lock(&mMutex);
    mIsRendering = false;
    pthread_cond_signal(&mCondition);
    pthread_mutex_unlock(&mMutex);

    pthread_join(mStartThread, 0);
}

EGLDemo::~EGLDemo() {
    pthread_mutex_destroy(&mMutex);
    pthread_cond_destroy(&mCondition);

    if (mWindow) {
        ANativeWindow_release(mWindow);
        mWindow = nullptr;
    }

    if (mEGLCore) {
        delete mEGLCore;
        mEGLCore = nullptr;
    }
}