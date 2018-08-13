//
// Created by zzh on 2018/8/2 0002.
//

#ifndef AVGRAPHICS_VIDEOPLAYER_H
#define AVGRAPHICS_VIDEOPLAYER_H


#include <android/asset_manager.h>
#include "../common/Yuv.h"
#include "EGLDemo.h"

class YuvRenderer : public EGLDemo {
private:
    AAssetManager *mAssetManager;

    GLuint mTextures[3];
    GLint mTexWidth;
    GLint mTexHeight;
    GLuint mVaoId;
    GLuint mVboIds[3];
    GLfloat mMatrix[16];

    GLint mMatrixLoc;
    GLint mSamplerY;
    GLint mSamplerU;
    GLint mSamplerV;

    Yuv *mYuv;

private:
    bool doInit() override;

    void doDraw() override;

    void doStop() override;

public:
    YuvRenderer(ANativeWindow *window);

    ~YuvRenderer() override;

    void setAssetManager(AAssetManager *mAssetManager);

    void setMatrix(GLfloat *matrix);

    void setTexSize(int texWidth, int texHeight);

    void setYuv(Yuv *yuv);
};


#endif //AVGRAPHICS_VIDEOPLAYER_H
