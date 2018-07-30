//
// Created by zzh on 2018/5/15 0015.
//

#ifndef OPENGLDEMO_OVAL_H
#define OPENGLDEMO_OVAL_H

#include "EGLDemo.h"

class Circle : public EGLDemo {
private:
    GLfloat *mVertices;
    GLuint mVertexNumer;

private:
    void createVertices();

protected:
    bool doInit() override;

    void doDraw() override;

public:
    Circle(ANativeWindow *window);

    ~Circle() override;
};


#endif //OPENGLDEMO_OVAL_H
