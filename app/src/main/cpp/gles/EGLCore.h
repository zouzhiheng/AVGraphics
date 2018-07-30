//
// Created by zzh on 2018/5/31 0031.
//

#ifndef AVGRAPHICS_EGLCORE_H
#define AVGRAPHICS_EGLCORE_H

#include <EGL/egl.h>
#include <GLES3/gl3.h>

class EGLCore {
private:
    EGLDisplay mDisplay;
    EGLSurface mSurface;
    EGLContext mContext;

public:
    EGLCore();

    ~EGLCore();

    GLboolean buildContext(ANativeWindow *window);

    void swapBuffer();

    void release();
};

#endif //AVGRAPHICS_EGLCORE_H
