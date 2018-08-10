//
// Created by zzh on 2018/7/31 0031.
//

#ifndef AVGRAPHICS_GLCAMERA_H
#define AVGRAPHICS_GLCAMERA_H


#include <android/asset_manager.h>
#include "GLDemo.h"
#include "EGLCore.h"

class GLCamera : public GLDemo {
private:
    AAssetManager *mAssetManager;
    ANativeWindow *mWindow;
    GLuint mTextureId;
    GLint mTextureLoc;
    GLint mMatrixLoc;
    GLfloat mMatrix[16];

    EGLCore *mEGLCore;

public:
    GLCamera(ANativeWindow *window);

    virtual ~GLCamera();

    void setAssetManager(AAssetManager *assetManager);

    int init();

    void draw(GLfloat *matrix);

    void stop();
};


#endif //AVGRAPHICS_GLCAMERA_H
