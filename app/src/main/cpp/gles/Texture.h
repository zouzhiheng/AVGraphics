//
// Created by zzh on 2018/7/30 0030.
//

#ifndef AVGRAPHICS_TEXTURE_H
#define AVGRAPHICS_TEXTURE_H


#include "EGLDemo.h"

class Texture : public EGLDemo {
private:
    GLuint mTexId;
    GLint mTexWidth;
    GLint mTexHeight;

    GLint mMatrixLoc;
    GLint mFilterTypeLoc;
    GLint mFilterColorLoc;
    GLint mSamplerLoc;

    GLint mFilterType;
    GLfloat *mFilterColor;
    uint8_t *mPixel;
    GLfloat *mMatrix;
    AAssetManager *mAssetManager;

protected:
    bool doInit() override;

    void doDraw() override;

    void doStop() override;

public:
    Texture(ANativeWindow *window);

    ~Texture() override;

    void setTexWidth(GLint mTexWidth);

    void setTexHeight(GLint mTexHeight);

    void setFilterType(GLint mFilterType);

    void setFilterColor(GLfloat *mFilterColor);

    void setPixel(uint8_t *mPixel);

    void setMatrix(GLfloat *mMatrix);

    void setAssetManager(AAssetManager *mAssetManager);
};


#endif //AVGRAPHICS_TEXTURE_H
