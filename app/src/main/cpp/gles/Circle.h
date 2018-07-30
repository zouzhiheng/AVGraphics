//
// Created by Administrator on 2018/5/15 0015.
//

#ifndef OPENGLDEMO_OVAL_H
#define OPENGLDEMO_OVAL_H

#include "Shape.h"

class Circle : public Shape {
private:
    GLfloat *mVertices;
    GLuint mVertexNumer;

public:
    Circle(ANativeWindow *window);

    ~Circle() override;

    void createVertices();

    bool doInit() override;

    void doDraw() override;
};


#endif //OPENGLDEMO_OVAL_H
