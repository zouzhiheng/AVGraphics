//
// Created by zzh on 2018/5/30 0030.
//

#ifndef OPENGLDEMO_SHAPE_RENDERER_H
#define OPENGLDEMO_SHAPE_RENDERER_H

#include <sys/types.h>
#include <android/native_window.h>
#include "GLDemo.h"
#include "EGLCore.h"

class EGLDemo : public GLDemo {
private:
    EGLCore *mEGLCore;
    ANativeWindow *mWindow;

    pthread_t mGLThread;
    pthread_mutex_t mMutex;
    pthread_cond_t mCondition;
    bool mIsRendering;
    bool mIsInitialized;

private:
    bool init();

    void renderLoop();

    void drawAndSwapBuffer();

    void stopDrawing();

    virtual bool doInit() = 0;

    virtual void doDraw() = 0;

    virtual void doStop() = 0;

public:
    EGLDemo(ANativeWindow *window);

    virtual ~EGLDemo();

    bool start();

    void draw();

    virtual void stop();

    friend void *glThreadFunc(void *arg);
};

#endif //OPENGLDEMO_SHAPE_RENDERER_H
