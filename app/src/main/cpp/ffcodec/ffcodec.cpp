//
// Created by Administrator on 2018/8/13 0013.
//

#include <jni.h>

extern "C"
JNIEXPORT jint JNICALL
Java_com_steven_avgraphics_module_av_FFCodec__1decode(JNIEnv *env, jclass type, jstring srcFile_,
                                                      jstring yuvDst_, jstring pcmDst_) {
    const char *srcFile = env->GetStringUTFChars(srcFile_, 0);
    const char *yuvDst = env->GetStringUTFChars(yuvDst_, 0);
    const char *pcmDst = env->GetStringUTFChars(pcmDst_, 0);

    // TODO

    env->ReleaseStringUTFChars(srcFile_, srcFile);
    env->ReleaseStringUTFChars(yuvDst_, yuvDst);
    env->ReleaseStringUTFChars(pcmDst_, pcmDst);
}