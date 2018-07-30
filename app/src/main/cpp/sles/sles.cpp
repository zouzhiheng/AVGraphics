#include <jni.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include "AssetAudioPlayer.h"
#include "AudioRecorder.h"
#include "log.h"
#include "BQAudioPlayer.h"

AssetAudioPlayer *assetAudioPlayer = nullptr;
BQAudioPlayer *bqAudioPlayer = nullptr;
AudioRecorder *audioRecorder = nullptr;

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

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_OpenSLActivity__1startPlayPcm(JNIEnv *env, jclass type,
                                                                  jstring filePath_) {
    const char *filePath = env->GetStringUTFChars(filePath_, 0);

    if (bqAudioPlayer) {
        bqAudioPlayer->stop();
        delete bqAudioPlayer;
    }
    bqAudioPlayer = new BQAudioPlayer(filePath);
    bqAudioPlayer->start();

    env->ReleaseStringUTFChars(filePath_, filePath);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_OpenSLActivity__1stopPlayPcm(JNIEnv *env, jclass type) {
    if (!bqAudioPlayer) {
        return;
    }
    bqAudioPlayer->stop();
    delete bqAudioPlayer;
    bqAudioPlayer = nullptr;
}
