//
// Created by zzh on 2018/5/14 0014.
//

#include "glutil.h"
#include "Triangle.h"

const static char *VERTEX_SHADER = ""
        "#version 300 es\n"
        "layout(location=0) in vec4 aColor;\n"
        "layout(location=1) in vec4 aPosition;\n"
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

const static GLfloat VERTICES[] = {
        -0.5f, 0.0f, 0.0f,
        0.5f, 0.0f, 0.0f,
        0.0f, 0.5f, 0.0f
};

const static GLfloat COLORS[] = {
        0.0f, 1.0f, 1.0f, 1.0f
};

static const GLuint ATTRIB_COLOR = 0;
static const GLuint ATTRIB_POSITION = 1;
static const GLsizei VERTEX_COUNT = 3;

void Triangle::init() {
    mProgram = loadProgram(VERTEX_SHADER, FRAGMENT_SHADER);
    glClearColor(ClearRed, ClearGreen, ClearBlue, ClearAlpha);
}

void Triangle::draw() {
    glViewport(0, 0, mWidth, mHeight);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(mProgram);

    glVertexAttrib4fv(ATTRIB_COLOR, COLORS);

    glVertexAttribPointer(ATTRIB_POSITION, VERTEX_COUNT, GL_FLOAT, GL_FALSE, 0, VERTICES);
    glEnableVertexAttribArray(ATTRIB_POSITION);
    glDrawArrays(GL_TRIANGLES, 0, VERTEX_COUNT);
    glDisableVertexAttribArray(ATTRIB_POSITION);
}
