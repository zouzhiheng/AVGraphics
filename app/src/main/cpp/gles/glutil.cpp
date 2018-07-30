//
// Created by zzh on 2018/5/11 0011.
//

#include "glutil.h"

#include <android/log.h>
#include <malloc.h>
#include <cstring>

#define LOG_TAG "glutil"
#define LOGI(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

std::string *readShaderFromAsset(AAssetManager *manager, const char *fileName) {
    AAssetDir *dir = AAssetManager_openDir(manager, "");
    const char *file = nullptr;
    std::string *result = new std::string;
    while ((file = AAssetDir_getNextFileName(dir)) != nullptr) {
        if (strcmp(file, fileName) == 0) {
            AAsset *asset = AAssetManager_open(manager, file, AASSET_MODE_STREAMING);
            char buf[1024];
            int nb_read = 0;
            while ((nb_read = AAsset_read(asset, buf, 1024)) > 0) {
                result->append(buf, (unsigned long) nb_read);
            }
            AAsset_close(asset);
            break;
        }
    }
    AAssetDir_close(dir);
    return result;
}

GLuint loadProgram(const char *vertexShaderStr, const char *fragmentShaderStr) {
    GLuint program = glCreateProgram();
    if (program == 0) {
        LOGE("create program failed");
        return 0;
    }

    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vertexShaderStr);
    GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragmentShaderStr);
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    // 链接
    glLinkProgram(program);

    GLint linked;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLint infoLen = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1) {
            char *infoLog = (char *) malloc(sizeof(char) * infoLen);
            glGetProgramInfoLog(program, infoLen, nullptr, infoLog);
            LOGE("loadProgram failed: %s", infoLog);
            free(infoLog);
        }

        glDeleteProgram(program);
        return 0;
    }

    return program;
}

GLuint loadShader(GLenum type, const char *shaderSrc) {
    GLuint shader = glCreateShader(type);
    if (shader == 0) {
        return 0;
    }

    // 加载 shader 源码
    glShaderSource(shader, 1, &shaderSrc, nullptr);

    // 编译
    glCompileShader(shader);

    // 检查编译状态
    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLint infoLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1) {
            char *infoLog = (char *) malloc(sizeof(char) * infoLen);
            glGetShaderInfoLog(shader, infoLen, nullptr, infoLog);
            LOGE("load %s shader failed: \n%s", type == GL_VERTEX_SHADER ? "vertex" : "fragment",
                 infoLog);
            free(infoLog);
        }

        glDeleteShader(shader);
        return 0;
    }

    return shader;
}
