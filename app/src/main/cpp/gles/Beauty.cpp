//
// Created by Administrator on 2018/7/31 0031.
//

#include <cstring>
#include <log.h>
#include "Beauty.h"
#include "glutil.h"
#include <GLES2/gl2ext.h>

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

const static GLushort INDICES[] = {
        0, 1, 2,
        1, 2, 3
};

const static GLuint ATTRIB_POSITION = 0;
const static GLuint ATTRIB_TEX_COORD = 1;
const static GLuint VERTEX_POS_SIZE = 2;
const static GLuint INDEX_NUMBER = 6;
const static GLuint TEX_COORD_POS_SIZE = 2;

Beauty::Beauty() : mEGLCore(new EGLCore), mWindow(nullptr), mTexOes(0), mTex2D(0), mFbo(0),
                   mMatrixLoc(0), mTexLoc(0), mParamsLoc(0), mBrightnessLoc(0),
                   mSingleStepOffsetLoc(0), mVao(0), mPboReadIndex(0), nPboMapIndex(-1) {
    memset(mVboIds, 0, sizeof(mVboIds));
    memset(mPboIds, 0, sizeof(mPboIds));
}

Beauty::~Beauty() {
    if (mWindow) {
        ANativeWindow_release(mWindow);
        mWindow = nullptr;
    }

    if (mEGLCore) {
        delete mEGLCore;
        mEGLCore = nullptr;
    }
}

int Beauty::init(AAssetManager *manager, ANativeWindow *window, int width, int height) {
    mWindow = window;
    resize(width, height);

    if (!mEGLCore->buildContext(window)) {
        LOGE("buildContext failed");
        return -1;
    }

    std::string *vShader = readShaderFromAsset(manager, "beauty.vert");
    std::string *fShader = readShaderFromAsset(manager, "beauty.frag");

    mProgram = loadProgram(vShader->c_str(), fShader->c_str());
    if (!mProgram) {
        LOGE("loadProgram failed!");
        return -1;
    }

    // 设置默认帧缓冲区纹理
    glGenTextures(1, &mTexOes);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, mTexOes);
    glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);

    initVbo();
    initVao();
    initPbo();
    if (!initFbo()) {
        return -1;
    }

    mMatrixLoc = glGetUniformLocation(mProgram, "mMatrix");
    mTexLoc = glGetUniformLocation(mProgram, "sTexture");
    mParamsLoc = glGetUniformLocation(mProgram, "params");
    mBrightnessLoc = glGetUniformLocation(mProgram, "brightness");
    mSingleStepOffsetLoc = glGetUniformLocation(mProgram, "singleStepOffset");

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    delete vShader;
    delete fShader;

    return mTexOes;
}

void Beauty::initVbo() {
    // 缓存顶点坐标、纹理坐标、索引数据到缓冲区中
    glGenBuffers(3, mVboIds);
    glBindBuffer(GL_ARRAY_BUFFER, mVboIds[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(VERTICES), VERTICES, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, mVboIds[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(TEX_COORDS), TEX_COORDS, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVboIds[2]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(INDICES), INDICES, GL_STATIC_DRAW);
}

void Beauty::initVao() {
    glGenVertexArrays(1, &mVao);
    // 使用缓冲区的数据设置顶点属性，并绑定至 vao
    glBindVertexArray(mVao);
    glBindBuffer(GL_ARRAY_BUFFER, mVboIds[0]);
    glEnableVertexAttribArray(ATTRIB_POSITION);
    glVertexAttribPointer(ATTRIB_POSITION, VERTEX_POS_SIZE, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, mVboIds[1]);
    glEnableVertexAttribArray(ATTRIB_TEX_COORD);
    glVertexAttribPointer(ATTRIB_TEX_COORD, TEX_COORD_POS_SIZE, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVboIds[2]);

    glBindVertexArray(0);
}

void Beauty::initPbo() {
    // 生成 pbo
    glGenBuffers(2, mPboIds);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, mPboIds[0]);
    glBufferData(GL_PIXEL_PACK_BUFFER, mWidth * mHeight * 4, nullptr, GL_DYNAMIC_COPY);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, mPboIds[1]);
    glBufferData(GL_PIXEL_PACK_BUFFER, mWidth * mHeight * 4, nullptr, GL_DYNAMIC_COPY);
}

bool Beauty::initFbo() {
    const GLenum attachment = GL_COLOR_ATTACHMENT0;

    // 设置离屛帧缓冲区纹理
    glGenTextures(1, &mTex2D);
    glBindTexture(GL_TEXTURE_2D, mTex2D);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mWidth, mHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenFramebuffers(1, &mFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, mFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, mTex2D, 0);
    glDrawBuffers(1, &attachment);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LOGE("glCheckFramebufferStatus failed: %d", glGetError());
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}

void Beauty::draw(GLfloat *matrix, GLfloat beauty, GLfloat tone, GLfloat bright, bool recording) {
    const GLenum attachment = GL_COLOR_ATTACHMENT0;

    // 渲染离屛帧缓冲区
    glBindFramebuffer(GL_FRAMEBUFFER, mFbo);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDrawBuffers(1, &attachment);

    glViewport(0, 0, mWidth, mHeight);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(mProgram);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mTex2D);
    glUniform1i(mTexLoc, 0);

    glUniformMatrix4fv(mMatrixLoc, 1, GL_FALSE, matrix);
    glUniform4fv(mParamsLoc, 1, getParams(beauty, tone));
    glUniform1f(mBrightnessLoc, getBright(bright));
    glUniform2fv(mSingleStepOffsetLoc, 1, getSingleStepOffset(mWidth, mHeight));

    // 从 vao 中读取数据并渲染
    glBindVertexArray(mVao);
    glDrawElements(GL_TRIANGLES, INDEX_NUMBER, GL_UNSIGNED_SHORT, 0);
    glBindVertexArray(0);

    // 渲染默认缓冲区
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, mFbo);

    glReadBuffer(attachment);
    glBlitFramebuffer(0, 0, mWidth, mHeight,
                      0, 0, mWidth, mHeight,
                      GL_COLOR_BUFFER_BIT, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glFlush();
    mEGLCore->swapBuffer();
}

GLfloat *Beauty::getParams(const GLfloat beauty, const GLfloat tone) {
    GLfloat *value = new GLfloat[4];
    value[0] = 1.6f - 1.2f * beauty;
    value[1] = 1.3f - 0.6f * beauty;
    value[2] = -0.2f + 0.6f * tone;
    value[3] = -0.2f + 0.6f * tone;
    return value;
}

GLfloat Beauty::getBright(const GLfloat bright) {
    return 0.6f * (-0.5f + bright);
}

GLfloat *Beauty::getSingleStepOffset(const GLfloat width, const GLfloat height) {
    GLfloat *value = new GLfloat[2];
    value[0] = 2.0f / width;
    value[1] = 2.0f / height;
    return value;
}

void Beauty::stop() {
    glDeleteProgram(mProgram);
    glDeleteTextures(1, &mTexOes);
    glDeleteTextures(1, &mTex2D);
    glDeleteBuffers(2, mPboIds);
    glDeleteBuffers(1, &mVao);
    glDeleteBuffers(3, mVboIds);
    glDeleteFramebuffers(1, &mFbo);
}