//
// Created by Administrator on 2018/5/15 0015.
//

#ifndef OPENGLDEMO_SQUARE_H
#define OPENGLDEMO_SQUARE_H

#include "EGLDemo.h"

class Square : public EGLDemo {
private:
    GLuint mVboIds[2];
    GLuint mVaoId;

public:
    Square(ANativeWindow *window);

    ~Square() override;

    bool doInit() override;

    void doDraw() override;

    void doStop() override;
};

#endif //OPENGLDEMO_SQUARE_H
