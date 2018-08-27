//
// Created by Administrator on 2018/8/23 0023.
//

#include <cstring>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <log.h>
#include "Watermark.h"
#include "glutil.h"

const static GLfloat VERTICES[] = {
        -1.0f, -1.0f,
        -1.0f, 1.0f,
        1.0f, 1.0f,
        1.0f, -1.0f
};

const static GLfloat CAMERA_COORDS[] = {
        0.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f,
        1.0f, 0.0f,
};

const static GLfloat WATERMARK_COORD[] = {
        0.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
};

const static GLushort INDICES[] = {
        0, 1, 2,
        0, 2, 3
};

const static GLuint ATTRIB_POSITION = 0;
const static GLuint ATTRIB_CAMERA_COORD = 1;
const static GLuint ATTRIB_WATERMARK_COORD = 2;
const static GLuint VERTEX_POS_SIZE = 2;
const static GLuint CAMERA_COORD_SIZE = 2;
const static GLuint WATERMARK_COORD_SIZE = 2;
const static GLuint INDEX_NUMBER = 6;

Watermark::Watermark(ANativeWindow *window)
        : mWindow(window), mEGLCore(new EGLCore()), mTextureOes(0), mAssetManager(nullptr),
          mCameraMatrixLoc(0), mCameraTextureLoc(0), mWatermarkMatrixLoc(0),
          mWatermarkTextureLoc(0), mWatermarkWidth(0), mWatermarkHeight(0),
          mWatermarkPixel(nullptr) {

}

Watermark::~Watermark() {
    if (mWindow) {
        ANativeWindow_release(mWindow);
        mWindow = nullptr;
    }

    if (mEGLCore) {
        delete mEGLCore;
        mEGLCore = nullptr;
    }

    if (mWatermarkPixel) {
        delete mWatermarkPixel;
        mWatermarkPixel = nullptr;
    }

    mAssetManager = nullptr;
}

void Watermark::setAssetManager(AAssetManager *assetManager) {
    mAssetManager = assetManager;
}

void Watermark::setWatermark(uint8_t *watermarkPixel, size_t length, GLint width,
                                 GLint height) {
    mWatermarkWidth = width;
    mWatermarkHeight = height;
    if (!mWatermarkPixel) {
        mWatermarkPixel = new uint8_t[length];
    }
    memcpy(mWatermarkPixel, watermarkPixel, length);
}

int Watermark::init() {
    if (!mEGLCore->buildContext(mWindow)) {
        return -1;
    }

    std::string *vShader = readShaderFromAsset(mAssetManager, "watermark.vert");
    std::string *fShader = readShaderFromAsset(mAssetManager, "watermark.frag");
    mProgram = loadProgram(vShader->c_str(), fShader->c_str());

    glGenTextures(1, &mTextureOes);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, mTextureOes);
    glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);

    glGenTextures(1, &mTexture2D);
    glBindTexture(GL_TEXTURE_2D, mTexture2D);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mWatermarkWidth, mWatermarkHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, mWatermarkPixel);

    mCameraMatrixLoc = glGetUniformLocation(mProgram, "mCameraMatrix");
    mWatermarkMatrixLoc = glGetUniformLocation(mProgram, "mWatermarkMatrix");
    mCameraTextureLoc = glGetUniformLocation(mProgram, "sCameraTexture");
    mWatermarkTextureLoc = glGetUniformLocation(mProgram, "sWatermarkTexture");

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    delete vShader;
    delete fShader;

    return mTextureOes;
}

void Watermark::draw(GLfloat *cameraMatrix, GLfloat *watermarkMatrix) {
    glViewport(0, 0, mWidth, mHeight);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(mProgram);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, mTextureOes);
    glUniform1i(mCameraTextureLoc, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mTexture2D);
    glUniform1i(mWatermarkTextureLoc, 1);

    glUniformMatrix4fv(mCameraMatrixLoc, 1, GL_FALSE, cameraMatrix);
    glUniformMatrix4fv(mWatermarkMatrixLoc, 1, GL_FALSE, watermarkMatrix);

    glEnableVertexAttribArray(ATTRIB_POSITION);
    glVertexAttribPointer(ATTRIB_POSITION, VERTEX_POS_SIZE, GL_FLOAT, GL_FALSE, 0, VERTICES);

    glEnableVertexAttribArray(ATTRIB_CAMERA_COORD);
    glVertexAttribPointer(ATTRIB_CAMERA_COORD, CAMERA_COORD_SIZE, GL_FLOAT, GL_FALSE, 0,
                          CAMERA_COORDS);

    glEnableVertexAttribArray(ATTRIB_WATERMARK_COORD);
    glVertexAttribPointer(ATTRIB_WATERMARK_COORD, WATERMARK_COORD_SIZE, GL_FLOAT, GL_FALSE, 0,
                          WATERMARK_COORD);

//    glDrawArrays(GL_TRIANGLE_STRIP, 0, VERTEX_NUM);
    glDrawElements(GL_TRIANGLES, INDEX_NUMBER, GL_UNSIGNED_SHORT, INDICES);

    glDisableVertexAttribArray(ATTRIB_POSITION);
    glDisableVertexAttribArray(ATTRIB_CAMERA_COORD);
    glDisableVertexAttribArray(ATTRIB_WATERMARK_COORD);

    glFlush();
    mEGLCore->swapBuffer();
}

void Watermark::stop() {
    glDeleteTextures(1, &mTextureOes);
    glDeleteTextures(1, &mTexture2D);
    glDeleteProgram(mProgram);
    mEGLCore->release();
}
