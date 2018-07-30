//
// Created by zzh on 2018/5/11 0011.
//

#ifndef AVGRAPHICS_ESUTIL_H
#define AVGRAPHICS_ESUTIL_H

#include <string>
#include <GLES3/gl3.h>
#include <android/asset_manager.h>

#ifdef __cplusplus
extern "C" {
#endif

std::string *readShaderFromAsset(AAssetManager *manager, const char *fileName);

GLuint loadProgram(const char *vertexShaderStr, const char *fragmentShaderStr);

GLuint loadShader(GLenum type, const char *shaderSrc);

#ifdef  __cplusplus
}
#endif


#endif //AVGRAPHICS_ESUTIL_H
