//
// Created by zzh on 2018/8/2 0002.
//

#include <GLES3/gl3.h>
#include <cstring>
#include <log.h>
#include "YuvRenderer.h"
#include "glutil.h"

const static GLfloat VERTICES[] = {
        -1.0f, 1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f,
        1.0f, -1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
};

const static GLfloat TEX_COORDS[] = {
        0.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f,
        1.0f, 0.0f
};

const static GLushort INDICES[] = {
        0, 1, 2,
        0, 2, 3
};

const static GLuint ATTRIB_POSITION = 0;
const static GLuint ATTRIB_TEX_COORD = 1;
const static GLint VERTEX_POS_SIZE = 3;
const static GLint TEX_COORD_SIZE = 2;
const static GLsizei INDEX_NUMBER = 6;

YuvRenderer::YuvRenderer(ANativeWindow *window) : EGLDemo(window), mAssetManager(nullptr),
                                                  mTexWidth(0), mTexHeight(0),
                                                  mVaoId(0), mMatrixLoc(0), mSamplerY(0),
                                                  mSamplerU(0), mSamplerV(0),
                                                  mYuv(nullptr) {
    memset(mTextures, 0, sizeof(mTextures));
    memset(mVboIds, 0, sizeof(mVboIds));
    memset(mMatrix, 0, sizeof(mMatrix));
    mMatrix[0] = 1;
    mMatrix[5] = 1;
    mMatrix[10] = 1;
    mMatrix[15] = 1;
}

YuvRenderer::~YuvRenderer() {
    if (mYuv) {
        mYuv->release();
        delete mYuv;
        mYuv = nullptr;
    }

    mAssetManager = nullptr;
}

void YuvRenderer::setAssetManager(AAssetManager *assetManager) {
    YuvRenderer::mAssetManager = assetManager;
}

void YuvRenderer::setMatrix(GLfloat *matrix) {
    memcpy(mMatrix, matrix, sizeof(mMatrix));
}

void YuvRenderer::setTexSize(int texWidth, int texHeight) {
    mTexWidth = texWidth;
    mTexHeight = texHeight;
}

void YuvRenderer::setYuv(Yuv *yuv) {
    if (!mYuv) {
        mYuv = yuv->clone();
    }
    if (mYuv->width != yuv->width || mYuv->height != yuv->height) {
        delete mYuv;
        mYuv = yuv->clone();
    } else {
        memcpy(mYuv->bufY, yuv->bufY, (size_t) (mTexWidth * mTexHeight));
        memcpy(mYuv->bufU, yuv->bufU, (size_t) (mTexWidth * mTexHeight / 4));
        memcpy(mYuv->bufV, yuv->bufV, (size_t) (mTexWidth * mTexHeight / 4));
    }
}

bool YuvRenderer::doInit() {
    std::string *vShader = readShaderFromAsset(mAssetManager, "yuv_renderer.vert");
    std::string *fShader = readShaderFromAsset(mAssetManager, "yuv_renderer.frag");

    mProgram = loadProgram(vShader->c_str(), fShader->c_str());

    mMatrixLoc = glGetUniformLocation(mProgram, "mMatrix");
    mSamplerY = glGetUniformLocation(mProgram, "yTexture");
    mSamplerU = glGetUniformLocation(mProgram, "uTexture");
    mSamplerV = glGetUniformLocation(mProgram, "vTexture");

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glGenTextures(3, mTextures);
    glBindTexture(GL_TEXTURE_2D, mTextures[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, mTexWidth, mTexHeight, 0, GL_LUMINANCE,
                 GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, mTextures[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, mTexWidth / 2, mTexHeight / 2, 0, GL_LUMINANCE,
                 GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, mTextures[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, mTexWidth / 2, mTexHeight / 2, 0, GL_LUMINANCE,
                 GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenBuffers(3, mVboIds);
    glBindBuffer(GL_ARRAY_BUFFER, mVboIds[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(VERTICES), VERTICES, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, mVboIds[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(TEX_COORDS), TEX_COORDS, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVboIds[2]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(INDICES), INDICES, GL_STATIC_DRAW);

    glGenVertexArrays(1, &mVaoId);
    glBindVertexArray(mVaoId);

    glBindBuffer(GL_ARRAY_BUFFER, mVboIds[0]);
    glEnableVertexAttribArray(ATTRIB_POSITION);
    glVertexAttribPointer(ATTRIB_POSITION, VERTEX_POS_SIZE, GL_FLOAT, GL_FALSE,
                          sizeof(GLfloat) * VERTEX_POS_SIZE, 0);

    glBindBuffer(GL_ARRAY_BUFFER, mVboIds[1]);
    glEnableVertexAttribArray(ATTRIB_TEX_COORD);
    glVertexAttribPointer(ATTRIB_TEX_COORD, TEX_COORD_SIZE, GL_FLOAT, GL_FALSE,
                          sizeof(GLfloat) * TEX_COORD_SIZE, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVboIds[2]);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    delete vShader;
    delete fShader;

    return true;
}

void YuvRenderer::doDraw() {
    glViewport(0, 0, mWidth, mHeight);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(mProgram);

    glUniformMatrix4fv(mMatrixLoc, 1, GL_FALSE, mMatrix);

    if (!mYuv) {
        LOGW("YuvRenderer doDraw failed: yuv data have not assigned");
        return;
    }
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mTextures[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, mTexWidth, mTexHeight, 0, GL_LUMINANCE,
                 GL_UNSIGNED_BYTE, mYuv->bufY);
    glUniform1i(mSamplerY, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mTextures[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, mTexWidth / 2, mTexHeight / 2, 0, GL_LUMINANCE,
                 GL_UNSIGNED_BYTE, mYuv->bufU);
    glUniform1i(mSamplerU, 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, mTextures[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, mTexWidth / 2, mTexHeight / 2, 0, GL_LUMINANCE,
                 GL_UNSIGNED_BYTE, mYuv->bufV);
    glUniform1i(mSamplerV, 2);

    glBindVertexArray(mVaoId);
    glDrawElements(GL_TRIANGLES, INDEX_NUMBER, GL_UNSIGNED_SHORT, 0);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void YuvRenderer::doStop() {
    glDeleteBuffers(3, mVboIds);
    glDeleteVertexArrays(1, &mVaoId);
    glDeleteTextures(3, mTextures);
}
