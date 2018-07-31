//
// Created by zzh on 2018/7/30 0030.
//

#include <android/asset_manager_jni.h>
#include <string>
#include <log.h>
#include <cstring>
#include "Texture.h"
#include "glutil.h"

const static GLfloat VERTICES[] = {
        -1.0f, 1.0f, 0.0f,  // Position 0
        0.0f, 0.0f,        // TexCoord 0
        -1.0f, -1.0f, 0.0f,  // Position 1
        0.0f, 1.0f,        // TexCoord 1
        1.0f, -1.0f, 0.0f,  // Position 2
        1.0f, 1.0f,        // TexCoord 2
        1.0f, 1.0f, 0.0f,  // Position 3
        1.0f, 0.0f         // TexCoord 3
};

const static GLushort INDICES[] = {
        0, 1, 2,
        0, 2, 3
};

const static int ATTRIB_POSITION = 0;
const static int ATTRIB_TEX_COORD = 1;
const static int VERTEX_POS_SIZE = 3;
const static int COORDINATE_SIZE = 2;
const static int INDEX_NUMBER = 6;

Texture::Texture(ANativeWindow *window) : EGLDemo(window), mTexId(0), mTexWidth(0), mTexHeight(0),
                                          mMatrixLoc(0), mFilterTypeLoc(0), mFilterColorLoc(0),
                                          mSamplerLoc(0), mFilterType(0), mPixel(nullptr),
                                          mAssetManager(nullptr) {
    memset(mFilterColor, 0, sizeof(mFilterColor));
    memset(mMatrix, 0, sizeof(mMatrix));
    mMatrix[0] = 1;
    mMatrix[5] = 1;
    mMatrix[10] = 1;
    mMatrix[15] = 1;
}

Texture::~Texture() {
    if (mPixel) {
        delete mPixel;
        mPixel = nullptr;
    }

    mAssetManager = nullptr;
}

void Texture::setTexWidth(GLint texWidth) {
    mTexWidth = texWidth;
}

void Texture::setTexHeight(GLint texHeight) {
    mTexHeight = texHeight;
}

void Texture::setFilterType(GLint filterType) {
    mFilterType = filterType;
}

void Texture::setFilterColor(GLfloat *filterColor) {
    memcpy(mFilterColor, filterColor, sizeof(mFilterColor));
}

void Texture::setPixel(uint8_t *pixel, size_t dataLen) {
    if (mPixel) {
        delete mPixel;
    }
    mPixel = new uint8_t[dataLen];
    memcpy(mPixel, pixel, dataLen);
}

void Texture::setMatrix(GLfloat *matrix) {
    memcpy(mMatrix, matrix, sizeof(mMatrix));
}

void Texture::setAssetManager(AAssetManager *assetManager) {
    mAssetManager = assetManager;
}

bool Texture::doInit() {
    std::string *vShader = readShaderFromAsset(mAssetManager, "texture.vert");
    std::string *fShader = readShaderFromAsset(mAssetManager, "texture.frag");

    mProgram = loadProgram(vShader->c_str(), fShader->c_str());

    mMatrixLoc = glGetUniformLocation(mProgram, "mMatrix");
    mSamplerLoc = glGetUniformLocation(mProgram, "sTexture");
    mFilterTypeLoc = glGetUniformLocation(mProgram, "iFilterType");
    mFilterColorLoc = glGetUniformLocation(mProgram, "vFilterColor");

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glGenTextures(1, &mTexId);
    glBindTexture(GL_TEXTURE_2D, mTexId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mTexWidth, mTexHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 mPixel);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    delete vShader;
    delete fShader;

    return true;
}

void Texture::doDraw() {
    glViewport(0, 0, mWidth, mHeight);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(mProgram);

    glUniformMatrix4fv(mMatrixLoc, 1, GL_FALSE, mMatrix);
    glUniform1i(mFilterTypeLoc, mFilterType);
    glUniform3fv(mFilterColorLoc, 1, mFilterColor);

    GLsizei stride = sizeof(GLfloat) * (VERTEX_POS_SIZE + COORDINATE_SIZE);
    glEnableVertexAttribArray(ATTRIB_POSITION);
    glVertexAttribPointer(ATTRIB_POSITION, VERTEX_POS_SIZE, GL_FLOAT, GL_FALSE, stride, VERTICES);
    glVertexAttribPointer(ATTRIB_TEX_COORD, COORDINATE_SIZE, GL_FLOAT, GL_FALSE, stride, &VERTICES[3]);

    glEnableVertexAttribArray(ATTRIB_POSITION);
    glEnableVertexAttribArray(ATTRIB_TEX_COORD);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mTexId);
    glUniform1i(mSamplerLoc, 0);

    glDrawElements(GL_TRIANGLES, INDEX_NUMBER, GL_UNSIGNED_SHORT, INDICES);

    glDisableVertexAttribArray(ATTRIB_POSITION);
    glDisableVertexAttribArray(ATTRIB_TEX_COORD);
}

void Texture::doStop() {
    glDeleteTextures(1, &mTexId);
}
