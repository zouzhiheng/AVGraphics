//
// Created by Administrator on 2018/7/31 0031.
//

#ifndef AVGRAPHICS_BEAUTY_H
#define AVGRAPHICS_BEAUTY_H

#include <android/asset_manager.h>
#include <android/native_window.h>
#include "GLDemo.h"
#include "EGLCore.h"

class Beauty : GLDemo {
private:
    EGLCore *mEGLCore;
    ANativeWindow *mWindow;

    GLuint mTexOes;
    GLuint mTex2D;
    GLuint mFbo;
    GLuint mVboIds[3];
    GLuint mVao;

    GLuint mPboIds[2];
    GLint mPboReadIndex;
    GLint nPboMapIndex;

    GLint mMatrixLoc;
    GLint mTexLoc;
    GLint mParamsLoc;
    GLint mBrightnessLoc;
    GLint mSingleStepOffsetLoc;

private:
    void initVbo();

    void initVao();

    void initPbo();

    bool initFbo();

    GLfloat *getParams(const GLfloat beauty, const GLfloat tone);

    GLfloat getBright(const GLfloat bright);

    GLfloat *getSingleStepOffset(const GLfloat width, const GLfloat height);

public:
    Beauty();

    virtual ~Beauty();

    int init(AAssetManager *manager, ANativeWindow *window, int width, int height);

    void draw(GLfloat *matrix, GLfloat beauty, GLfloat tone, GLfloat bright, bool recording);

    void stop();
};


#endif //AVGRAPHICS_BEAUTY_H
