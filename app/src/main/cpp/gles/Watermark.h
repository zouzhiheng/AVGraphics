//
// Created by Administrator on 2018/8/23 0023.
//

#ifndef AVGRAPHICS_CAMERAEFFECT_H
#define AVGRAPHICS_CAMERAEFFECT_H


#include <android/native_window.h>
#include <android/asset_manager.h>
#include "GLDemo.h"
#include "EGLCore.h"

class Watermark : public GLDemo {
private:
    AAssetManager *mAssetManager;
    ANativeWindow *mWindow;

    EGLCore *mEGLCore;
    GLuint mTextureOes;
    GLuint mTexture2D;

    GLint mCameraMatrixLoc;
    GLint mCameraTextureLoc;
    GLint mWatermarkMatrixLoc;
    GLint mWatermarkTextureLoc;

    GLint mWatermarkWidth;
    GLint mWatermarkHeight;
    uint8_t *mWatermarkPixel;

public:
    Watermark(ANativeWindow *window);

    virtual ~Watermark();

    void setAssetManager(AAssetManager *assetManager);

    void setWatermark(uint8_t *watermarkPixel, size_t length, GLint width, GLint height);

    int init();

    void draw(GLfloat *cameraMatrix, GLfloat *watermarkMatrix);

    void stop();
};


#endif //AVGRAPHICS_CAMERAEFFECT_H
