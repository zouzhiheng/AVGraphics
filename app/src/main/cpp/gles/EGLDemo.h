//
// Created by Administrator on 2018/5/30 0030.
//

#ifndef OPENGLDEMO_SHAPE_RENDERER_H
#define OPENGLDEMO_SHAPE_RENDERER_H

#include <sys/types.h>
#include <android/native_window.h>
#include "GLDemo.h"
#include "EGLCore.h"

class EGLDemo : public GLDemo {
private:
    ANativeWindow *mWindow;

    pthread_t mStartThread;
    pthread_mutex_t mMutex;
    pthread_cond_t mCondition;
    bool mIsRendering;

protected:
    EGLCore *mEGLCore;

private:
    void renderLoop();

protected:
    virtual bool doInit() = 0;

    virtual void doDraw() = 0;

    virtual void doStop();

public:
    EGLDemo(ANativeWindow *window);

    virtual ~EGLDemo();

    void start();

    void draw();

    void stop();

    friend void *startThreadCallback(void *arg);
};

#endif //OPENGLDEMO_SHAPE_RENDERER_H
