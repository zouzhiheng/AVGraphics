//
// Created by zzh on 2018/7/31 0031.
//

#include <log.h>
#include "FboRenderer.h"
#include "glutil.h"

const static char *VERTEX_SHADER = ""
        "#version 300 es                            \n"
        "layout(location = 0) in vec4 a_position;   \n"
        "void main()                                \n"
        "{                                          \n"
        "   gl_Position = a_position;               \n"
        "}                                          \n";

const static char *FRAGMENT_SHADER = ""
        "#version 300 es                                     \n"
        "precision mediump float;                            \n"
        "layout(location = 0) out vec4 fragData0;            \n"
        "layout(location = 1) out vec4 fragData1;            \n"
        "layout(location = 2) out vec4 fragData2;            \n"
        "layout(location = 3) out vec4 fragData3;            \n"
        "void main()                                         \n"
        "{                                                   \n"
        "  fragData0 = vec4 ( 1, 0, 0, 1 );                  \n"
        "  fragData1 = vec4 ( 0, 1, 0, 1 );                  \n"
        "  fragData2 = vec4 ( 0, 0, 1, 1 );                  \n"
        "  fragData3 = vec4 ( 1, 1, 0, 1 );                  \n"
        "}                                                   \n";

const static GLfloat VERTICES[] = {
        -1.0f, 1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f,
        1.0f, -1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
};

const static GLushort INDICES[] = {
        0, 1, 2, 0, 2, 3
};

const static int BUFFER_COUNT = 4;
const static GLenum ATTACHMENTS[BUFFER_COUNT] = {
        GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1,
        GL_COLOR_ATTACHMENT2,
        GL_COLOR_ATTACHMENT3
};

FboRenderer::FboRenderer(ANativeWindow *window) : EGLDemo(window), mFrameBuffer(0) {

}

FboRenderer::~FboRenderer() {

}

bool FboRenderer::doInit() {
    mProgram = loadProgram(VERTEX_SHADER, FRAGMENT_SHADER);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    GLuint colorTexId[BUFFER_COUNT];

    GLint defaultFramebuffer = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &defaultFramebuffer);

    // 设置 fbo
    glGenFramebuffers(1, &mFrameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffer);

    // 设置四个输出 buffer 并附着到 fbo 中
    glGenTextures(BUFFER_COUNT, &colorTexId[0]);
    for (int i = 0; i < BUFFER_COUNT; ++i) {
        glBindTexture(GL_TEXTURE_2D, colorTexId[i]);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mWidth, mHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, ATTACHMENTS[i], GL_TEXTURE_2D,
                               colorTexId[i], 0);
    }

    glDrawBuffers(BUFFER_COUNT, ATTACHMENTS);

    if (GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
        LOGE("glCheckFramebufferStatus failed");
        return false;
    }

    // 恢复默认帧缓冲区
    glBindFramebuffer(GL_FRAMEBUFFER, (GLuint) defaultFramebuffer);

    return true;
}

void FboRenderer::doDraw() {
    GLint defaultFramebuffer = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &defaultFramebuffer);

    glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDrawBuffers(BUFFER_COUNT, ATTACHMENTS);

    glViewport(0, 0, mWidth, mHeight);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(mProgram);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), VERTICES);
    glEnableVertexAttribArray(0);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, INDICES);

    // 恢复默认帧缓冲区
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, (GLuint) defaultFramebuffer);
    // 设置 fbo 为读取源
    glBindFramebuffer(GL_READ_FRAMEBUFFER, mFrameBuffer);

    // 复制红色缓冲区到左下角
    glReadBuffer(ATTACHMENTS[0]);
    glBlitFramebuffer(0, 0, mWidth, mHeight,
                      0, 0, mWidth / 2, mHeight / 2,
                      GL_COLOR_BUFFER_BIT, GL_LINEAR);

    // 复制绿色缓冲区到右下角
    glReadBuffer(ATTACHMENTS[1]);
    glBlitFramebuffer(0, 0, mWidth, mHeight,
                      mWidth / 2, 0, mWidth, mHeight / 2,
                      GL_COLOR_BUFFER_BIT, GL_LINEAR);

    // 复制蓝色缓冲区到左上角
    glReadBuffer(ATTACHMENTS[2]);
    glBlitFramebuffer(0, 0, mWidth, mHeight,
                      0, mHeight / 2, mWidth / 2, mHeight,
                      GL_COLOR_BUFFER_BIT, GL_LINEAR);

    // 复制黄色缓冲区到右上角
    glReadBuffer(ATTACHMENTS[3]);
    glBlitFramebuffer(0, 0, mWidth, mHeight,
                      mWidth / 2, mHeight / 2, mWidth, mHeight,
                      GL_COLOR_BUFFER_BIT, GL_LINEAR);
}

void FboRenderer::doStop() {
    glDeleteFramebuffers(1, &mFrameBuffer);
}
