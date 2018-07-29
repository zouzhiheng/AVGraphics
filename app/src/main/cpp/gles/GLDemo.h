//
// Created by Steven on 2018/7/29.
//

#ifndef AVGRAPHICS_GLDEMO_H
#define AVGRAPHICS_GLDEMO_H

#include <GLES3/gl3.h>

#define ClearRed 0.66f
#define ClearGreen 0.66f
#define ClearBlue 0.66f
#define ClearAlpha 1.0f

class GLDemo {
protected:
    GLuint mProgram;
    GLint mWidth;
    GLint mHeight;

public:
    GLDemo() : mProgram(0), mWidth(0), mHeight(0) {

    }

    GLuint getProgram() const {
        return mProgram;
    }

    void setProgram(GLuint mProgram) {
        GLDemo::mProgram = mProgram;
    }

    GLint getWidth() const {
        return mWidth;
    }

    void setWidth(GLint mWidth) {
        GLDemo::mWidth = mWidth;
    }

    GLint getHeight() const {
        return mHeight;
    }

    void setHeight(GLint mHeight) {
        GLDemo::mHeight = mHeight;
    }
};

#endif //AVGRAPHICS_GLDEMO_H
