//
// Created by zzh on 2018/7/27 0027.
//

#include <assert.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <SLES/OpenSLES_Android.h>
#include "AssetAudioPlayer.h"
#include "log.h"

AssetAudioPlayer::AssetAudioPlayer(AAsset *asset)
        : mAudioEngine(new AudioEngine()), mPlayerObj(nullptr), mPlayer(nullptr),
          mVolume(nullptr), mMuteSolo(nullptr), mSeek(nullptr) {
    initPlayer(asset);
}

void AssetAudioPlayer::initPlayer(AAsset *asset) {
    SLresult result;

    off_t start, length;
    int fd = AAsset_openFileDescriptor(asset, &start, &length);
    assert(fd >= 0);
    AAsset_close(asset);

    // 配置音频数据源
    SLDataLocator_AndroidFD locFD = {SL_DATALOCATOR_ANDROIDFD, fd, start, length};
    SLDataFormat_MIME formatMime = {SL_DATAFORMAT_MIME, nullptr, SL_CONTAINERTYPE_UNSPECIFIED};
    SLDataSource audioSrc = {&locFD, &formatMime};

    // 配置音频数据输出池
    SLDataLocator_OutputMix locOutputMix = {SL_DATALOCATOR_OUTPUTMIX, mAudioEngine->outputMixObj};
    SLDataSink audioSink = {&locOutputMix, nullptr};

    const SLInterfaceID ids[3] = {SL_IID_SEEK, SL_IID_MUTESOLO, SL_IID_VOLUME};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
    result = (*mAudioEngine->engine)->CreateAudioPlayer(mAudioEngine->engine, &mPlayerObj,
                                                        &audioSrc, &audioSink, 3, ids, req);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    result = (*mPlayerObj)->Realize(mPlayerObj, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    result = (*mPlayerObj)->GetInterface(mPlayerObj, SL_IID_PLAY, &mPlayer);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    result = (*mPlayerObj)->GetInterface(mPlayerObj, SL_IID_SEEK, &mSeek);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    result = (*mPlayerObj)->GetInterface(mPlayerObj, SL_IID_MUTESOLO, &mMuteSolo);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    result = (*mPlayerObj)->GetInterface(mPlayerObj, SL_IID_VOLUME, &mVolume);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    result = (*mSeek)->SetLoop(mSeek, SL_BOOLEAN_TRUE, 0, SL_TIME_UNKNOWN);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;
}

void AssetAudioPlayer::start() {
    setPlayerState(SL_PLAYSTATE_PLAYING);
}

void AssetAudioPlayer::pause() {
    setPlayerState(SL_PLAYSTATE_PAUSED);
}

void AssetAudioPlayer::stop() {
    setPlayerState(SL_PLAYSTATE_STOPPED);
}

void AssetAudioPlayer::setPlayerState(SLuint32 state) {
    SLresult result;

    if (mPlayer) {
        result = (*mPlayer)->SetPlayState(mPlayer, state);
        assert(SL_RESULT_SUCCESS == result);
        (void) result;
    }
}

AssetAudioPlayer::~AssetAudioPlayer() {
    release();
}

void AssetAudioPlayer::release() {
    if (mPlayerObj) {
        (*mPlayerObj)->Destroy(mPlayerObj);
        mPlayerObj = nullptr;
        mPlayer = nullptr;
        mSeek = nullptr;
        mMuteSolo = nullptr;
        mVolume = nullptr;
    }

    if (mAudioEngine) {
        delete mAudioEngine;
        mAudioEngine = nullptr;
    }

    LOGI("AssetAudioPlayer released");
}

