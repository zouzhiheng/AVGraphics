//
// Created by zzh on 2018/7/31 0031.
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
    GLuint mVboIds[3];
    GLuint mVao;

    GLuint mPboIds[2];
    GLint mPboReadIndex;
    GLint mPboMapIndex;

    GLint mMatrixLoc;
    GLint mTexLoc;
    GLint mParamsLoc;
    GLint mBrightnessLoc;
    GLint mSingleStepOffsetLoc;

private:
    void initVbo();

    void initVao();

    void initPbo();

    GLfloat *getParams(const GLfloat beauty, const GLfloat saturate);

    GLfloat getBright(const GLfloat bright);

    GLfloat *getSingleStepOffset(const GLfloat width, const GLfloat height);

public:
    Beauty();

    virtual ~Beauty();

    int init(AAssetManager *manager, ANativeWindow *window, int width, int height);

    void draw(GLfloat *matrix, GLfloat beauty, GLfloat saturate, GLfloat bright, bool recording);

    void stop();
};


#endif //AVGRAPHICS_BEAUTY_H
