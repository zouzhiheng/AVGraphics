//
// Created by zzh on 2018/7/31 0031.
//

#ifndef AVGRAPHICS_FBORENDERER_H
#define AVGRAPHICS_FBORENDERER_H


#include "EGLDemo.h"

class FboRenderer : public EGLDemo {
private:
    GLuint mFrameBuffer;

private:
    bool doInit() override;

    void doDraw() override;

    void doStop() override;

public:
    FboRenderer(ANativeWindow *window);

    ~FboRenderer() override;
};


#endif //AVGRAPHICS_FBORENDERER_H
