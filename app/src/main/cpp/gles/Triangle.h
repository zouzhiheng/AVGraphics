//
// Created by Administrator on 2018/5/15 0015.
//

#ifndef OPENGLDEMO_COLORFUL_TRIANGLE_H
#define OPENGLDEMO_COLORFUL_TRIANGLE_H

#include "Shape.h"

class Triangle : public Shape {
public:
    Triangle(ANativeWindow *window);

    bool init() override;

    void doDraw() override;

    void release() override;
};

#endif //OPENGLDEMO_COLORFUL_TRIANGLE_H
