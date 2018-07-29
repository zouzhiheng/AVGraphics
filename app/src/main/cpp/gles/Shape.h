//
// Created by Administrator on 2018/5/30 0030.
//

#ifndef OPENGLDEMO_SHAPE_RENDERER_H
#define OPENGLDEMO_SHAPE_RENDERER_H

#include <sys/types.h>
#include <android/native_window.h>
#include "GLDemo.h"
#include "EGLCore.h"

class Shape : public GLDemo {
protected:
    ANativeWindow *mWindow;
    EGLCore *mEGLCore;

    pthread_t mThreadId;
    pthread_mutex_t mMutex;
    pthread_cond_t mCondition;
    bool mIsRendering;

private:
    void renderLoop();

protected:
    virtual bool init() = 0;

    virtual void doDraw() = 0;

    virtual void release();

public:
    Shape(ANativeWindow *window);

    void resize(int width, int height);

    void start();

    void draw();

    void stop();

    friend void *threadStartCallback(void *arg);
};

#endif //OPENGLDEMO_SHAPE_RENDERER_H
