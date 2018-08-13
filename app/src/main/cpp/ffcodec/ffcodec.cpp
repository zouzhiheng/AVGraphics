//
// Created by Administrator on 2018/8/13 0013.
//

#include <jni.h>
#include <string>
#include "../common/log.h"
#include "Decoder.h"

using namespace std;

struct DecodeParams {
    string srcFilePath;
    string dstVideoPath;
    string dstAudioPath;
};

void *decodeThreadFunc(void *arg);

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_module_av_FFCodec__1decode(JNIEnv *env, jclass type, jstring srcFile_,
                                                      jstring yuvDst_, jstring pcmDst_) {
    const char *srcFile = env->GetStringUTFChars(srcFile_, 0);
    const char *yuvDst = env->GetStringUTFChars(yuvDst_, 0);
    const char *pcmDst = env->GetStringUTFChars(pcmDst_, 0);

    LOGI("decode file: %s\nyuv data save to :%s\npcm data save to: %s", srcFile, yuvDst, pcmDst);
    DecodeParams *param = new DecodeParams;
    param->srcFilePath = srcFile;
    param->dstVideoPath = yuvDst;
    param->dstAudioPath = pcmDst;
    pthread_t decodeThread;
    pthread_create(&decodeThread, nullptr, decodeThreadFunc, param);

    env->ReleaseStringUTFChars(srcFile_, srcFile);
    env->ReleaseStringUTFChars(yuvDst_, yuvDst);
    env->ReleaseStringUTFChars(pcmDst_, pcmDst);
}

void *decodeThreadFunc(void *arg) {
    DecodeParams *param = (DecodeParams *) arg;
    Decoder *decoder = new Decoder();
    decoder->decode(param->srcFilePath.c_str(), param->dstVideoPath.c_str(),
                    param->dstAudioPath.c_str());

    delete decoder;
    delete param;

    return (void *) 1;
}