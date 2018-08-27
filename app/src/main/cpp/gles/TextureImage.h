//
// Created by zzh on 2018/7/30 0030.
//

#ifndef AVGRAPHICS_TEXTURE_H
#define AVGRAPHICS_TEXTURE_H


#include "EGLDemo.h"

class TextureImage : public EGLDemo {
private:
    GLuint mTexId;
    GLint mTexWidth;
    GLint mTexHeight;

    GLint mMatrixLoc;
    GLint mFilterTypeLoc;
    GLint mFilterColorLoc;
    GLint mSamplerLoc;

    GLint mFilterType;
    GLfloat mFilterColor[3];
    uint8_t *mPixel;
    GLfloat mMatrix[16];
    AAssetManager *mAssetManager;

private:
    bool doInit() override;

    void doDraw() override;

    void doStop() override;

public:
    TextureImage(ANativeWindow *window);

    ~TextureImage() override;

    void setTexWidth(GLint texWidth);

    void setTexHeight(GLint texHeight);

    void setFilterType(GLint filterType);

    void setFilterColor(GLfloat *filterColor);

    void setPixel(uint8_t *pixel, size_t dataLen);

    void setMatrix(GLfloat *mMatrix);

    void setAssetManager(AAssetManager *assetManager);
};


#endif //AVGRAPHICS_TEXTURE_H
