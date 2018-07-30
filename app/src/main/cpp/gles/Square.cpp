//
// Created by Administrator on 2018/5/14 0014.
//
#include <GLES3/gl3.h>
#include "glutil.h"
#include "Square.h"

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

const static GLfloat VERTICES[] = {
        -0.5f, 0.5f, 0.0f,
        0.0f, 1.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, 0.0f,
        1.0f, 0.0f, 0.0f, 1.0f,
        0.5f, -0.5f, 0.0f,
        0.0f, 0.0f, 1.0f, 1.0f,
        0.5f, 0.5f, 0.0f,
        1.0f, 0.0f, 1.0f, 1.0f
};

const static GLushort INDICES[] = {
        0, 1, 2,
        0, 2, 3
};

const static int ATTRIB_POSITION = 0;
const static int ATTRIB_COLOR = 1;
const static int VERTEX_POS_SIZE = 3;
const static int VERTEX_COLOR_SIZE = 4;
const static int VERTEX_STRIDE = sizeof(GLfloat) * (VERTEX_POS_SIZE + VERTEX_COLOR_SIZE);
const static int INDEX_NUMBER = 6;

Square::Square(ANativeWindow *window) : Shape(window) {

}

Square::~Square() {

}

bool Square::doInit() {
    Shape::doInit();

    mProgram = loadProgram(VERTEX_SHADER, FRAGMENT_SHADER);

    glGenBuffers(2, mVboIds);
    glBindBuffer(GL_ARRAY_BUFFER, mVboIds[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(VERTICES), VERTICES, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVboIds[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(INDICES), INDICES, GL_STATIC_DRAW);

    glGenVertexArrays(1, &mVaoId);
    // Bind the VAO and then setup the vertex attributes
    glBindVertexArray(mVaoId);

    glBindBuffer(GL_ARRAY_BUFFER, mVboIds[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVboIds[1]);

    glEnableVertexAttribArray(ATTRIB_POSITION);
    glEnableVertexAttribArray(ATTRIB_COLOR);

    GLint offset = 0;
    glVertexAttribPointer(ATTRIB_POSITION, VERTEX_POS_SIZE, GL_FLOAT, GL_FALSE,
                          VERTEX_STRIDE, (const GLvoid *) offset);
    offset += VERTEX_POS_SIZE * sizeof(GLfloat);
    glVertexAttribPointer(ATTRIB_COLOR, VERTEX_COLOR_SIZE, GL_FLOAT, GL_FALSE,
                          VERTEX_STRIDE, (const GLvoid *) offset);

    glBindVertexArray(0);

    glClearColor(ClearRed, ClearGreen, ClearBlue, ClearAlpha);

    return true;
}

void Square::doDraw() {
    glViewport(0, 0, mWidth, mHeight);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(mProgram);

    glBindVertexArray(mVaoId);
    glDrawElements(GL_TRIANGLES, INDEX_NUMBER, GL_UNSIGNED_SHORT, (const GLvoid *) 0);
    glBindVertexArray(0);

    glFlush();
    mEGLCore->swapBuffer();
}

void Square::doStop() {
    glDeleteVertexArrays(1, &mVaoId);
    glDeleteBuffers(2, mVboIds);
    Shape::doStop();
}

