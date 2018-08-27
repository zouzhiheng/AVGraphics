//
// Created by zzh on 2018/7/30 0030.
//

#include <android/asset_manager_jni.h>
#include <string>
#include <log.h>
#include <cstring>
#include "TextureImage.h"
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

const static GLuint ATTRIB_POSITION = 0;
const static GLuint ATTRIB_TEX_COORD = 1;
const static GLint VERTEX_POS_SIZE = 3;
const static GLint TEX_COORD_SIZE = 2;
const static GLsizei INDEX_NUMBER = 6;

TextureImage::TextureImage(ANativeWindow *window) : EGLDemo(window), mTexId(0), mTexWidth(0), mTexHeight(0),
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

TextureImage::~TextureImage() {
    if (mPixel) {
        delete mPixel;
        mPixel = nullptr;
    }

    mAssetManager = nullptr;
}

void TextureImage::setTexWidth(GLint texWidth) {
    mTexWidth = texWidth;
}

void TextureImage::setTexHeight(GLint texHeight) {
    mTexHeight = texHeight;
}

void TextureImage::setFilterType(GLint filterType) {
    mFilterType = filterType;
}

void TextureImage::setFilterColor(GLfloat *filterColor) {
    memcpy(mFilterColor, filterColor, sizeof(mFilterColor));
}

void TextureImage::setPixel(uint8_t *pixel, size_t dataLen) {
    if (mPixel) {
        delete mPixel;
    }
    mPixel = new uint8_t[dataLen];
    memcpy(mPixel, pixel, dataLen);
}

void TextureImage::setMatrix(GLfloat *matrix) {
    memcpy(mMatrix, matrix, sizeof(mMatrix));
}

void TextureImage::setAssetManager(AAssetManager *assetManager) {
    mAssetManager = assetManager;
}

bool TextureImage::doInit() {
    std::string *vShader = readShaderFromAsset(mAssetManager, "texture_image.vert");
    std::string *fShader = readShaderFromAsset(mAssetManager, "texture_image.frag");

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

void TextureImage::doDraw() {
    glViewport(0, 0, mWidth, mHeight);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(mProgram);

    glUniformMatrix4fv(mMatrixLoc, 1, GL_FALSE, mMatrix);
    glUniform1i(mFilterTypeLoc, mFilterType);
    glUniform3fv(mFilterColorLoc, 1, mFilterColor);

    GLsizei stride = sizeof(GLfloat) * (VERTEX_POS_SIZE + TEX_COORD_SIZE);
    glEnableVertexAttribArray(ATTRIB_POSITION);
    glEnableVertexAttribArray(ATTRIB_TEX_COORD);
    glVertexAttribPointer(ATTRIB_POSITION, VERTEX_POS_SIZE, GL_FLOAT, GL_FALSE, stride, VERTICES);
    glVertexAttribPointer(ATTRIB_TEX_COORD, TEX_COORD_SIZE, GL_FLOAT, GL_FALSE, stride,
                          &VERTICES[3]);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mTexId);
    glUniform1i(mSamplerLoc, 0);

    glDrawElements(GL_TRIANGLES, INDEX_NUMBER, GL_UNSIGNED_SHORT, INDICES);

    glDisableVertexAttribArray(ATTRIB_POSITION);
    glDisableVertexAttribArray(ATTRIB_TEX_COORD);
}

void TextureImage::doStop() {
    glDeleteTextures(1, &mTexId);
}
