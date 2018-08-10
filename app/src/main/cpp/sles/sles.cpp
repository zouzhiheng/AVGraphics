//
// Created by zzh on 2018/7/27 0002.
//

#include <jni.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <pthread.h>
#include "AssetAudioPlayer.h"
#include "AudioRecorder.h"
#include "log.h"
#include "BQAudioPlayer.h"

AssetAudioPlayer *assetAudioPlayer = nullptr;

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_steven_avgraphics_activity_OpenSLActivity__1startPlayMp3(JNIEnv *env, jclass type,
                                                                  jobject assetManager,
                                                                  jstring filename_) {
    const char *filename = env->GetStringUTFChars(filename_, 0);
    if (!filename) {
        LOGE("get utf chars from jstring failed");
        return JNI_FALSE;
    }

    AAssetManager *manager = AAssetManager_fromJava(env, assetManager);
    if (!manager) {
        LOGE("get asset manager from java failed");
        return JNI_FALSE;
    }
    AAsset *asset = AAssetManager_open(manager, filename, AASSET_MODE_UNKNOWN);

    env->ReleaseStringUTFChars(filename_, filename);

    if (asset == nullptr) {
        LOGE("open asset manager failed");
        return JNI_FALSE;
    }

    if (assetAudioPlayer) {
        assetAudioPlayer->stop();
        delete assetAudioPlayer;
    }
    assetAudioPlayer = new AssetAudioPlayer(asset);
    assetAudioPlayer->start();

    return JNI_TRUE;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_OpenSLActivity__1stopPlayMp3(JNIEnv *env, jclass type) {
    if (!assetAudioPlayer) {
        return;
    }
    assetAudioPlayer->stop();
    delete assetAudioPlayer;
    assetAudioPlayer = nullptr;
}

AudioRecorder *audioRecorder = nullptr;

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_steven_avgraphics_activity_OpenSLActivity__1startRecord(JNIEnv *env, jclass type,
                                                                 jstring filePath_) {
    const char *filePath = env->GetStringUTFChars(filePath_, 0);

    if (audioRecorder) {
        audioRecorder->stop();
        delete audioRecorder;
    }
    audioRecorder = new AudioRecorder(filePath);

    env->ReleaseStringUTFChars(filePath_, filePath);

    return (jboolean) audioRecorder->start();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_OpenSLActivity__1stopRecord(JNIEnv *env, jclass type) {
    if (!audioRecorder) {
        return;
    }
    audioRecorder->stop();
    delete audioRecorder;
    audioRecorder = nullptr;
}

BQAudioPlayer *bqAudioPlayer = nullptr;
bool isPlaying = false;
FILE *pcmFile = nullptr;

void *playThreadFunc(void *arg);

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_OpenSLActivity__1startPlayPcm(JNIEnv *env, jclass type,
                                                                  jstring filePath_) {
    const char *filePath = env->GetStringUTFChars(filePath_, 0);

    if (bqAudioPlayer) {
        bqAudioPlayer->release();
        delete bqAudioPlayer;
    }
    bqAudioPlayer = new BQAudioPlayer(48000, SAMPLE_FORMAT_16, 1);
    bqAudioPlayer->init();
    pcmFile = fopen(filePath, "r");
    isPlaying = true;
    pthread_t playThread;
    pthread_create(&playThread, nullptr, playThreadFunc, 0);

    env->ReleaseStringUTFChars(filePath_, filePath);
}

void *playThreadFunc(void *arg) {
    LOGI("BQAudioPlayer started");
    const int bufferSize = 2048;
    short buffer[bufferSize];
    while (isPlaying && !feof(pcmFile)) {
        fread(buffer, 1, bufferSize, pcmFile);
        bqAudioPlayer->enqueueSample(buffer, bufferSize);
    }
    LOGI("BQAudioPlayer stopped");

    return 0;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_OpenSLActivity__1stopPlayPcm(JNIEnv *env, jclass type) {
    isPlaying = false;
    if (bqAudioPlayer) {
        bqAudioPlayer->release();
        delete bqAudioPlayer;
        bqAudioPlayer = nullptr;
    }

    if (pcmFile) {
        fclose(pcmFile);
        pcmFile = nullptr;
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_VideoPlayActivity__1startSL(JNIEnv *env, jclass type,
                                                                jint sampleRate, jint samleFormat,
                                                                jint channels) {
    if (bqAudioPlayer) {
        bqAudioPlayer->release();
        delete bqAudioPlayer;
    }
    bqAudioPlayer = new BQAudioPlayer(sampleRate, samleFormat, channels);
    if (!bqAudioPlayer->init()) {
        bqAudioPlayer->release();
        delete bqAudioPlayer;
        bqAudioPlayer = nullptr;
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_VideoPlayActivity__1writeSL(JNIEnv *env, jclass type,
                                                                jbyteArray data_, jint length) {
    if (!bqAudioPlayer) {
        return;
    }
    jbyte *data = env->GetByteArrayElements(data_, NULL);
    bqAudioPlayer->enqueueSample(data, (size_t) length);
    env->ReleaseByteArrayElements(data_, data, 0);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_VideoPlayActivity__1stopSL(JNIEnv *env, jclass type) {
    if (bqAudioPlayer) {
        bqAudioPlayer->release();
        delete bqAudioPlayer;
        bqAudioPlayer = nullptr;
    }
}