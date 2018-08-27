//
//
// Created by zzh on 2018/7/27 0002.
//

#include <jni.h>
#include <android/native_window_jni.h>
#include <log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <mutex>
#include <thread>
#include "Square.h"
#include "Triangle.h"
#include "Circle.h"
#include "TextureImage.h"
#include "FboRenderer.h"
#include "GLCamera.h"
#include "Beauty.h"
#include "YuvRenderer.h"
#include "AVModel.h"
#include "format.h"
#include "Watermark.h"

using namespace std;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

// --- JniTriangleActivity
Triangle *triangle = nullptr;
mutex gMutex;

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_gles_JniTriangleActivity__1init(JNIEnv *env, jclass type) {
    if (triangle) {
        delete triangle;
    }
    triangle = new Triangle();
    triangle->init();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_gles_JniTriangleActivity__1resize(JNIEnv *env, jclass type,
                                                                      jint width, jint height) {
    if (!triangle) {
        LOGE("triangle does not initialized");
        return;
    }
    triangle->resize(width, height);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_gles_JniTriangleActivity__1draw(JNIEnv *env, jclass type) {
    if (!triangle) {
        LOGE("triangle does not initialized");
        return;
    }
    triangle->draw();
}


// --- EGLCircleActivity
Circle *circle = nullptr;

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_gles_EGLCircleActivity__1init(JNIEnv *env, jclass type,
                                                                  jobject surface, jint width,
                                                                  jint height) {
    unique_lock<mutex> lock(gMutex);
    if (circle) {
        circle->stop();
        delete circle;
    }
    ANativeWindow *window = ANativeWindow_fromSurface(env, surface);
    circle = new Circle(window);
    circle->resize(width, height);
    circle->start();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_gles_EGLCircleActivity__1draw(JNIEnv *env, jclass type) {
    unique_lock<mutex> lock(gMutex);
    if (!circle) {
        LOGE("draw error, circle is null");
        return;
    }
    circle->draw();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_gles_EGLCircleActivity__1release(JNIEnv *env, jclass type) {
    unique_lock<mutex> lock(gMutex);
    if (circle) {
        circle->stop();
        delete circle;
        circle = nullptr;
    }
}


// --- VaoVboActivity
Square *square = nullptr;

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_gles_VaoVboActivity__1init(JNIEnv *env, jclass type,
                                                               jobject surface, jint width,
                                                               jint height) {
    unique_lock<mutex> lock(gMutex);
    if (square) {
        square->stop();
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
    unique_lock<mutex> lock(gMutex);
    if (!square) {
        LOGE("draw error, square is null");
        return;
    }
    square->draw();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_gles_VaoVboActivity__1release(JNIEnv *env, jclass type) {
    unique_lock<mutex> lock(gMutex);
    if (square) {
        square->stop();
        delete square;
        square = nullptr;
    }
}


// --- MatrixTransformActivity

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_gles_MatrixTransformActivity__1init(JNIEnv *env, jclass type,
                                                                        jobject surface, jint width,
                                                                        jint height) {
    unique_lock<mutex> lock(gMutex);
    if (square) {
        square->stop();
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
    unique_lock<mutex> lock(gMutex);
    if (!square) {
        LOGE("draw error, square is null");
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
    unique_lock<mutex> lock(gMutex);
    if (square) {
        square->stop();
        delete square;
        square = nullptr;
    }
}


// --- TextureImageActivity
TextureImage *texture = nullptr;

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_gles_TextureImageActivity__1init(JNIEnv *env, jclass type,
                                                                jobject surface, jint texWidth,
                                                                jint texHeight, jbyteArray pixel_,
                                                                jint pixelDataLen,
                                                                jobject assetManager) {
    jbyte *pixel = env->GetByteArrayElements(pixel_, NULL);

    unique_lock<mutex> lock(gMutex);
    if (texture) {
        texture->stop();
        delete texture;
    }
    ANativeWindow *window = ANativeWindow_fromSurface(env, surface);
    AAssetManager *manager = AAssetManager_fromJava(env, assetManager);
    texture = new TextureImage(window);
    texture->setTexWidth(texWidth);
    texture->setTexHeight(texHeight);
    texture->setPixel((uint8_t *) pixel, (size_t) pixelDataLen);
    texture->setAssetManager(manager);
    texture->resize(texWidth, texHeight);
    texture->start();

    env->ReleaseByteArrayElements(pixel_, pixel, 0);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_gles_TextureImageActivity__1resize(JNIEnv *env, jclass type,
                                                                  jint width, jint height) {
    unique_lock<mutex> lock(gMutex);
    if (!texture) {
        LOGE("resize error, texture is null");
        return;
    }
    texture->resize(width, height);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_gles_TextureImageActivity__1draw(JNIEnv *env, jclass type,
                                                                jfloatArray matrix_,
                                                                jint filterType,
                                                                jfloatArray filterColor_) {
    unique_lock<mutex> lock(gMutex);
    if (!texture) {
        LOGE("draw error, texture is null");
        return;
    }
    jfloat *matrix = env->GetFloatArrayElements(matrix_, NULL);
    jfloat *filterColor = env->GetFloatArrayElements(filterColor_, NULL);

    texture->setMatrix(matrix);
    texture->setFilterType(filterType);
    texture->setFilterColor(filterColor);
    texture->draw();

    env->ReleaseFloatArrayElements(matrix_, matrix, 0);
    env->ReleaseFloatArrayElements(filterColor_, filterColor, 0);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_gles_TextureImageActivity__1release(JNIEnv *env, jclass type) {
    unique_lock<mutex> lock(gMutex);
    if (texture) {
        texture->stop();
        delete texture;
        texture = nullptr;
    }
}


// --- FboActivity
FboRenderer *fboRenderer = nullptr;

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_gles_FboActivity__1init(JNIEnv *env, jclass type,
                                                            jobject surface, jint width,
                                                            jint height) {
    unique_lock<mutex> lock(gMutex);
    if (fboRenderer) {
        fboRenderer->stop();
        delete fboRenderer;
    }
    ANativeWindow *window = ANativeWindow_fromSurface(env, surface);
    fboRenderer = new FboRenderer(window);
    fboRenderer->resize(width, height);
    fboRenderer->start();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_gles_FboActivity__1draw(JNIEnv *env, jclass type) {
    unique_lock<mutex> lock(gMutex);
    if (!fboRenderer) {
        LOGE("draw error, fboRenderer is null");
        return;
    }
    fboRenderer->draw();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_gles_FboActivity__1release(JNIEnv *env, jclass type) {
    unique_lock<mutex> lock(gMutex);
    if (fboRenderer) {
        fboRenderer->stop();
        delete fboRenderer;
        fboRenderer = nullptr;
    }
}


// --- GLCameraActivity
GLCamera *glCamera = nullptr;

extern "C"
JNIEXPORT jint JNICALL
Java_com_steven_avgraphics_activity_gles_GLCameraActivity__1init(JNIEnv *env, jclass type,
                                                                 jobject surface, jint width,
                                                                 jint height,
                                                                 jobject assetManager) {
    unique_lock<mutex> lock(gMutex);
    if (glCamera) {
        glCamera->stop();
        delete glCamera;
    }
    ANativeWindow *window = ANativeWindow_fromSurface(env, surface);
    AAssetManager *manager = AAssetManager_fromJava(env, assetManager);
    glCamera = new GLCamera(window);
    glCamera->setAssetManager(manager);
    glCamera->resize(width, height);

    return glCamera->init();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_gles_GLCameraActivity__1draw(JNIEnv *env, jclass type,
                                                                 jfloatArray matrix_) {
    jfloat *matrix = env->GetFloatArrayElements(matrix_, NULL);

    unique_lock<mutex> lock(gMutex);
    if (!glCamera) {
        LOGE("draw error, glCamera is null");
        return;
    }
    glCamera->draw(matrix);

    env->ReleaseFloatArrayElements(matrix_, matrix, 0);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_gles_GLCameraActivity__1release(JNIEnv *env, jclass type) {
    unique_lock<mutex> lock(gMutex);
    if (glCamera) {
        glCamera->stop();
        delete glCamera;
        glCamera = nullptr;
    }
}


// --- WatermarkActivity

Watermark *watermark = nullptr;

extern "C"
JNIEXPORT jint JNICALL
Java_com_steven_avgraphics_activity_gles_WatermarkActivity__1init(JNIEnv *env, jclass type,
                                                                  jobject surface, jint width,
                                                                  jint height, jbyteArray data_,
                                                                  jint dataLen, jint watermarkWidth,
                                                                  jint watermarkHeight,
                                                                  jobject manager_) {
    jbyte *data = env->GetByteArrayElements(data_, NULL);

    unique_lock<mutex> lock(gMutex);
    if (watermark) {
        watermark->stop();
        delete watermark;
    }
    ANativeWindow *window = ANativeWindow_fromSurface(env, surface);
    AAssetManager *manager = AAssetManager_fromJava(env, manager_);
    watermark = new Watermark(window);
    watermark->setAssetManager(manager);
    watermark->resize(width, height);
    watermark->setWatermark((uint8_t *) data, (size_t) dataLen, watermarkWidth, watermarkHeight);

    env->ReleaseByteArrayElements(data_, data, 0);

    return watermark->init();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_gles_WatermarkActivity__1draw(JNIEnv *env, jclass type,
                                                                     jfloatArray cameraMatrix_,
                                                                     jfloatArray watermarkMatrix_) {
    jfloat *cameraMatrix = env->GetFloatArrayElements(cameraMatrix_, NULL);
    jfloat *watermarkMatrix = env->GetFloatArrayElements(watermarkMatrix_, NULL);

    unique_lock<mutex> lock(gMutex);
    if (!watermark) {
        LOGE("draw error, watermark is null");
        return;
    }
    watermark->draw(cameraMatrix, watermarkMatrix);

    env->ReleaseFloatArrayElements(cameraMatrix_, cameraMatrix, 0);
    env->ReleaseFloatArrayElements(watermarkMatrix_, watermarkMatrix, 0);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_gles_WatermarkActivity__1release(JNIEnv *env, jclass type) {
    unique_lock<mutex> lock(gMutex);
    if (watermark) {
        watermark->stop();
        delete watermark;
        watermark = nullptr;
    }
}

// --- CameraPreviewView
Beauty *beauty = nullptr;

extern "C"
JNIEXPORT jint JNICALL
Java_com_steven_avgraphics_view_CameraPreviewView__1init(JNIEnv *env, jclass type, jobject surface,
                                                         jint width, jint height, jobject manager) {
    unique_lock<mutex> lock(gMutex);
    if (beauty) {
        beauty->stop();
        delete beauty;
    }
    beauty = new Beauty();
    ANativeWindow *window = ANativeWindow_fromSurface(env, surface);
    AAssetManager *assetManager = AAssetManager_fromJava(env, manager);
    int textureId = beauty->init(assetManager, window, width, height);

    return textureId;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_view_CameraPreviewView__1draw(JNIEnv *env, jclass type,
                                                         jfloatArray matrix_, jfloat beautyLevel,
                                                         jfloat saturate, jfloat bright,
                                                         jboolean recording) {
    unique_lock<mutex> lock(gMutex);
    if (!beauty) {
        LOGE("draw error, beauty is null");
        return;
    }
    jfloat *matrix = env->GetFloatArrayElements(matrix_, NULL);
    beauty->draw(matrix, beautyLevel, saturate, bright, recording);
    env->ReleaseFloatArrayElements(matrix_, matrix, 0);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_view_CameraPreviewView__1stop(JNIEnv *env, jclass type) {
    unique_lock<mutex> lock(gMutex);
    if (beauty) {
        beauty->stop();
        delete beauty;
        beauty = nullptr;
    }
}


// --- VideoPlayActivity
YuvRenderer *yuvRenderer = nullptr;

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_VideoPlayActivity__1startGL(JNIEnv *env, jclass type,
                                                                jobject surface, jint width,
                                                                jint height, jint imgWidth,
                                                                jint imgHeight, jint frameRate,
                                                                jobject manager) {
    unique_lock<mutex> lock(gMutex);
    if (yuvRenderer) {
        yuvRenderer->stop();
        delete yuvRenderer;
    }
    AAssetManager *assetManager = AAssetManager_fromJava(env, manager);
    ANativeWindow *window = ANativeWindow_fromSurface(env, surface);
    yuvRenderer = new YuvRenderer(window);
    yuvRenderer->setAssetManager(assetManager);
    yuvRenderer->setTexSize(imgWidth, imgHeight);
    yuvRenderer->resize(width, height);
    yuvRenderer->start();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_VideoPlayActivity__1drawGL(JNIEnv *env, jclass type,
                                                               jbyteArray pixel_, jint length,
                                                               jint imgWidth, jint imgHeight,
                                                               jint pixelFormat,
                                                               jfloatArray matrix_) {
    unique_lock<mutex> lock(gMutex);
    if (!yuvRenderer) {
        LOGE("yuvRenderer is null");
        return;
    }
    jbyte *pixel = env->GetByteArrayElements(pixel_, NULL);
    jfloat *matrix = env->GetFloatArrayElements(matrix_, NULL);

    AVModel *model = new AVModel();
    model->image = (uint8_t *) pixel;
    model->imageLen = length;
    model->flag = MODEL_FLAG_VIDEO;
    model->pixelFormat = pixelFormat;
    model->width = imgWidth;
    model->height = imgHeight;
    Yuv *yuv = convertToI420(model);

    if (yuv) {
        yuvRenderer->setYuv(yuv);
        yuvRenderer->setMatrix(matrix);
        yuvRenderer->draw();
    }

    env->ReleaseByteArrayElements(pixel_, pixel, 0);
    env->ReleaseFloatArrayElements(matrix_, matrix, 0);
    delete yuv;
    model->image = nullptr;
    delete model;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_VideoPlayActivity__1stopGL(JNIEnv *env, jclass type) {
    unique_lock<mutex> lock(gMutex);
    if (yuvRenderer) {
        yuvRenderer->stop();
        delete yuvRenderer;
        yuvRenderer = nullptr;
    }
    LOGI("video player stopped");
}

#pragma clang diagnostic pop