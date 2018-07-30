//
// Created by Administrator on 2018/5/14 0014.
//
#include <GLES3/gl3.h>
#include <jni.h>
#include <android/native_window_jni.h>
#include <android/log.h>
#include <cstring>
#include "glutil.h"
#include "Square.h"

#define LOG_TAG "Circle"
#define LOGI(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

const static char *VERTEX_SHADER = ""
        "#version 300 es\n"
        "layout(location=0) in vec4 aPosition;\n"
        "layout(location=1) in vec4 aColor;\n"
        "uniform mat4 mMatrix;\n"
        "out vec4 vColor;\n"
        "void main() {\n"
        "   vColor = aColor;\n"
        "   gl_Position = mMatrix * aPosition;\n"
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

Square::Square(ANativeWindow *window) : EGLDemo(window), mVaoId(0), mMatrixLoc(0) {
    memset(mVboIds, 0, sizeof(mVboIds));
    // 初始化为单位矩阵
    memset(mMatrix, 0, sizeof(mMatrix));
    mMatrix[0] = 1;
    mMatrix[5] = 1;
    mMatrix[10] = 1;
    mMatrix[15] = 1;
}

Square::~Square() {

}

void Square::setMatrix(GLfloat *matrx) {
    memcpy(mMatrix, matrx, sizeof(mMatrix));
}

bool Square::doInit() {
    EGLDemo::doInit();

    mProgram = loadProgram(VERTEX_SHADER, FRAGMENT_SHADER);

    mMatrixLoc = glGetUniformLocation(mProgram, "mMatrix");

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

    glUniformMatrix4fv(mMatrixLoc, 1, GL_FALSE, mMatrix);

    glBindVertexArray(mVaoId);
    glDrawElements(GL_TRIANGLES, INDEX_NUMBER, GL_UNSIGNED_SHORT, (const GLvoid *) 0);
    glBindVertexArray(0);

    glFlush();
    mEGLCore->swapBuffer();
}

void Square::doStop() {
    glDeleteVertexArrays(1, &mVaoId);
    glDeleteBuffers(2, mVboIds);
    EGLDemo::doStop();
}

Square *square = nullptr;

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_gles_VaoVboActivity__1init(JNIEnv *env, jclass type,
                                                               jobject surface, jint width,
                                                               jint height) {
    if (square) {
        delete square;
    }
    ANativeWindow *window = ANativeWindow_fromSurface(env, surface);
    square = new Square(window);
    square->resize(width, height);
    square->start();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_gles_VaoVboActivity__1draw(JNIEnv *env, jclass type) {
    if (square == nullptr) {
        LOGE("draw error, shape is null");
        return;
    }
    square->draw();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_gles_VaoVboActivity__1release(JNIEnv *env, jclass type) {
    if (square) {
        square->stop();
        delete square;
        square = nullptr;
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_gles_MatrixTransformActivity__1init(JNIEnv *env, jclass type,
                                                                        jobject surface, jint width,
                                                                        jint height) {
    if (square) {
        delete square;
    }
    ANativeWindow *window = ANativeWindow_fromSurface(env, surface);
    square = new Square(window);
    square->resize(width, height);
    square->start();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_gles_MatrixTransformActivity__1draw(JNIEnv *env, jclass type,
                                                                        jfloatArray matrix_) {
    if (square == nullptr) {
        LOGE("draw error, shape is null");
        return;
    }

    jfloat *matrix = env->GetFloatArrayElements(matrix_, NULL);

    square->setMatrix(matrix);
    square->draw();

    env->ReleaseFloatArrayElements(matrix_, matrix, 0);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_gles_MatrixTransformActivity__1release(JNIEnv *env,
                                                                           jclass type) {
    if (square) {
        square->stop();
        delete square;
        square = nullptr;
    }
}
