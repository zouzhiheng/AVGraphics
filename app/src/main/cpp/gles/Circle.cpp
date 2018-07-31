//
// Created by zzh on 2018/5/15 0015.
//

#include <cstring>
#include "Circle.h"
#include "glutil.h"
#include <cmath>
#include <vector>

using namespace std;

#define PI 3.14159265359

const static char *VERTEX_SHADER = ""
        "#version 300 es\n"
        "layout(location=0) in vec4 aPosition;\n"
        "layout(location=1) in vec4 aColor;\n"
        "out vec4 vColor;\n"
        "void main() {\n"
        "   vColor = aColor;\n"
        "   gl_Position = aPosition;\n"
        "}\n";

const static char *FRAGMENT_SHADER = ""
        "#version 300 es\n"
        "precision mediump float;\n"
        "in vec4 vColor;\n"
        "out vec4 fragColor;\n"
        "void main() {\n"
        "   fragColor = vColor;\n"
        "}\n";

const static GLfloat COLORS[] = {
        1.0f, 0.0f, 1.0f, 1.0f
};

const static GLuint ATTRIB_POSITION = 0;
const static GLuint ATTRIB_COLOR = 1;
const static GLint VERTEX_POS_SIZE = 3;

Circle::Circle(ANativeWindow *window) : EGLDemo(window) {
    createVertices();
}

void Circle::createVertices() {
    vector<GLfloat> vec;
    vec.push_back(0.0f);
    vec.push_back(0.0f);
    vec.push_back(0.0f);
    for (GLfloat i = 0; i < 360; ++i) {
        vec.push_back(0.5f * static_cast<GLfloat>(sin(i * PI / 180)));
        vec.push_back(0.5f * static_cast<GLfloat>(cos(i * PI / 180)));
        vec.push_back(0.0f);
    }
    GLuint size = static_cast<GLuint>(vec.size());
    GLfloat *vertices = new GLfloat[size + 3];
    for (int i = 0; i < size; i += 3) {
        vertices[i] = vec[i];
        vertices[i + 1] = vec[i + 1];
        vertices[i + 2] = vec[i + 2];
    }
    vertices[size] = vec[3];
    vertices[size + 1] = vec[4];
    vertices[size + 2] = vec[5];
    mVertexNumer = size / 3 + 1;
    mVertices = vertices;
}

bool Circle::doInit() {
    mProgram = loadProgram(VERTEX_SHADER, FRAGMENT_SHADER);
    glClearColor(ClearRed, ClearGreen, ClearBlue, ClearAlpha);
    return mProgram > 0;
}

void Circle::doDraw() {
    glViewport(0, 0, mWidth, mHeight);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(mProgram);

    glVertexAttrib4fv(ATTRIB_COLOR, COLORS);

    glEnableVertexAttribArray(ATTRIB_POSITION);
    glVertexAttribPointer(ATTRIB_POSITION, VERTEX_POS_SIZE, GL_FLOAT, GL_FALSE, 0, mVertices);
    glDrawArrays(GL_TRIANGLE_FAN, 0, mVertexNumer);
    glDisableVertexAttribArray(ATTRIB_POSITION);
}

void Circle::doStop() {

}

Circle::~Circle() {
    if (mVertices) {
        delete mVertices;
        mVertices = nullptr;
    }
}

