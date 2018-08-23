//
// Created by Administrator on 2018/8/23 0023.
//

#include <cstring>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include "CameraEffect.h"
#include "glutil.h"

const static GLfloat VERTICES[] = {
        -1.0f, 1.0f,
        1.0f, 1.0f,
        -1.0f, -1.0f,
        1.0f, -1.0f
};

const static GLfloat CAMERA_COORDS[] = {
        0.0f, 1.0f,
        1.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f
};

const static GLuint ATTRIB_POSITION = 0;
const static GLuint ATTRIB_CAMERA_COORD = 1;
const static GLuint VERTEX_NUM = 4;
const static GLuint VERTEX_POS_SIZE = 2;
const static GLuint CAMERA_COORD_SIZE = 2;

CameraEffect::CameraEffect(ANativeWindow *window) : mWindow(window), mEGLCore(new EGLCore()),
                                                    mAssetManager(nullptr), mCameraTextureLoc(0),
                                                    mWatermarkTextureLoc(0), mTextTextureLoc(0),
                                                    mMatrixLoc(0) {
    memset(mTextures, 0, sizeof(mTextures));
    memset(mMatrix, 0, sizeof(mMatrix));
    mMatrix[0] = 1;
    mMatrix[5] = 1;
    mMatrix[10] = 1;
    mMatrix[15] = 1;
}

CameraEffect::~CameraEffect() {
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

void CameraEffect::setAssetManager(AAssetManager *assetManager) {
    CameraEffect::mAssetManager = assetManager;
}

int CameraEffect::init() {if (!mEGLCore->buildContext(mWindow)) {
        return -1;
    }

    std::string *vShader = readShaderFromAsset(mAssetManager, "camera_effect.vert");
    std::string *fShader = readShaderFromAsset(mAssetManager, "camera_effect.frag");
    mProgram = loadProgram(vShader->c_str(), fShader->c_str());

    glGenTextures(3, mTextures);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, mTextures[0]);
    glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    mMatrixLoc = glGetUniformLocation(mProgram, "mMatrix");
    mCameraTextureLoc = glGetUniformLocation(mProgram, "sCameraTexture");

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    delete vShader;
    delete fShader;

    return mTextures[0];
}

void CameraEffect::draw(GLfloat *matrix) {
    glViewport(0, 0, mWidth, mHeight);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(mProgram);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, mTextures[0]);
    glUniform1i(mCameraTextureLoc, 0);

    glUniformMatrix4fv(mMatrixLoc, 1, GL_FALSE, matrix);

    glEnableVertexAttribArray(ATTRIB_POSITION);
    glVertexAttribPointer(ATTRIB_POSITION, VERTEX_POS_SIZE, GL_FLOAT, GL_FALSE, 0, VERTICES);
    glEnableVertexAttribArray(ATTRIB_CAMERA_COORD);
    glVertexAttribPointer(ATTRIB_CAMERA_COORD, CAMERA_COORD_SIZE, GL_FLOAT, GL_FALSE, 0, CAMERA_COORDS);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, VERTEX_NUM);

    glDisableVertexAttribArray(ATTRIB_POSITION);
    glDisableVertexAttribArray(ATTRIB_CAMERA_COORD);

    glFlush();
    mEGLCore->swapBuffer();
}

void CameraEffect::stop() {
    glDeleteTextures(3, mTextures);
    glDeleteProgram(mProgram);
    mEGLCore->release();
}
