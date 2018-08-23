//
// Created by Administrator on 2018/8/23 0023.
//

#ifndef AVGRAPHICS_CAMERAEFFECT_H
#define AVGRAPHICS_CAMERAEFFECT_H


#include <android/native_window.h>
#include <android/asset_manager.h>
#include "GLDemo.h"
#include "EGLCore.h"

class CameraEffect : public GLDemo {
private:
    AAssetManager *mAssetManager;
    ANativeWindow *mWindow;

    GLuint mTextures[3];
    GLint mCameraTextureLoc;
    GLint mWatermarkTextureLoc;
    GLint mTextTextureLoc;
    GLint mMatrixLoc;
    GLfloat mMatrix[16];

    EGLCore *mEGLCore;

public:
    CameraEffect(ANativeWindow *window);

    virtual ~CameraEffect();

    void setAssetManager(AAssetManager *assetManager);

    int init();

    void draw(GLfloat *matrix);

    void stop();
};


#endif //AVGRAPHICS_CAMERAEFFECT_H
