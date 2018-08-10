//
// Created by zzh on 2018/7/31 0031.
//

#include <string>
#include "GLCamera.h"
#include "glutil.h"
#include <GLES3/gl3.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <cstring>
#include <android/native_window.h>

const static GLfloat VERTICES[] = {
        -1.0f, 1.0f,
        1.0f, 1.0f,
        -1.0f, -1.0f,
        1.0f, -1.0f
};

const static GLfloat TEX_COORDS[] = {
        0.0f, 1.0f,
        1.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f
};

const static GLuint ATTRIB_POSITION = 0;
const static GLuint ATTRIB_TEXCOORD = 1;
const static GLuint VERTEX_NUM = 4;
const static GLuint VERTEX_POS_SIZE = 2;
const static GLuint TEX_COORD_POS_SIZE = 2;

GLCamera::GLCamera(ANativeWindow *window) : mWindow(window), mEGLCore(new EGLCore()),
                                            mAssetManager(nullptr), mTextureId(0), mTextureLoc(0),
                                            mMatrixLoc(0) {
    memset(mMatrix, 0, sizeof(mMatrix));
    mMatrix[0] = 1;
    mMatrix[5] = 1;
    mMatrix[10] = 1;
    mMatrix[15] = 1;
}

GLCamera::~GLCamera() {
    if (mWindow) {
        ANativeWindow_release(mWindow);
        mWindow = nullptr;
    }

    if (mEGLCore) {
        delete mEGLCore;
        mEGLCore = nullptr;
    }

    mAssetManager = nullptr;
}

void GLCamera::setAssetManager(AAssetManager *assetManager) {
    GLCamera::mAssetManager = assetManager;
}

int GLCamera::init() {
    if (!mEGLCore->buildContext(mWindow)) {
        return -1;
    }

    std::string *vShader = readShaderFromAsset(mAssetManager, "camera.vert");
    std::string *fShader = readShaderFromAsset(mAssetManager, "camera.frag");
    mProgram = loadProgram(vShader->c_str(), fShader->c_str());

    glGenTextures(1, &mTextureId);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, mTextureId);
    glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    mMatrixLoc = glGetUniformLocation(mProgram, "mMatrix");
    mTextureLoc = glGetUniformLocation(mProgram, "sTexture");

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    delete vShader;
    delete fShader;

    return mTextureId;
}

void GLCamera::draw(GLfloat *matrix) {
    glViewport(0, 0, mWidth, mHeight);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(mProgram);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, mTextureId);
    glUniform1i(mTextureLoc, 0);

    glUniformMatrix4fv(mMatrixLoc, 1, GL_FALSE, matrix);

    glEnableVertexAttribArray(ATTRIB_POSITION);
    glVertexAttribPointer(ATTRIB_POSITION, VERTEX_POS_SIZE, GL_FLOAT, GL_FALSE, 0, VERTICES);
    glEnableVertexAttribArray(ATTRIB_TEXCOORD);
    glVertexAttribPointer(ATTRIB_TEXCOORD, TEX_COORD_POS_SIZE, GL_FLOAT, GL_FALSE, 0, TEX_COORDS);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, VERTEX_NUM);

    glDisableVertexAttribArray(ATTRIB_POSITION);
    glDisableVertexAttribArray(ATTRIB_TEXCOORD);

    glFlush();
    mEGLCore->swapBuffer();
}

void GLCamera::stop() {
    glDeleteTextures(1, &mTextureId);
    glDeleteProgram(mProgram);
    mEGLCore->release();
}
