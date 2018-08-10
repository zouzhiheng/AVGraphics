//
// Created by zzh on 2018/7/27 0027.
//

#include <assert.h>
#include "BQAudioPlayer.h"
#include "log.h"
#include <pthread.h>

void playerCallback(SLAndroidSimpleBufferQueueItf bq, void *context);

BQAudioPlayer::BQAudioPlayer(int sampleRate, int sampleFormat, int channels)
        : mAudioEngine(new AudioEngine()), mPlayerObj(nullptr), mPlayer(nullptr),
          mBufferQueue(nullptr), mEffectSend(nullptr), mVolume(nullptr),
          mSampleRate((SLmilliHertz) sampleRate * 1000), mSampleFormat(sampleFormat),
          mChannels(channels), mBufSize(0), mIndex(0) {
    mMutex = PTHREAD_MUTEX_INITIALIZER;
    mBuffers[0] = nullptr;
    mBuffers[1] = nullptr;
}

BQAudioPlayer::~BQAudioPlayer() {

}

bool BQAudioPlayer::init() {
    SLresult result;

    SLDataLocator_AndroidSimpleBufferQueue locBufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    // channelMask: 位数和 channel 相等，0 代表 SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT
    SLDataFormat_PCM formatPcm = {SL_DATAFORMAT_PCM, (SLuint32) mChannels, mSampleRate,
                                  (SLuint32) mSampleFormat, (SLuint32) mSampleFormat,
                                  mChannels == 2 ? 0 : SL_SPEAKER_FRONT_CENTER,
                                  SL_BYTEORDER_LITTLEENDIAN};
    /*
     * Enable Fast Audio when possible:  once we set the same rate to be the native, fast audio path
     * will be triggered
     */
    if (mSampleRate) {
        formatPcm.samplesPerSec = mSampleRate;
    }
    SLDataSource audioSrc = {&locBufq, &formatPcm};

    SLDataLocator_OutputMix locOutpuMix = {SL_DATALOCATOR_OUTPUTMIX, mAudioEngine->outputMixObj};
    SLDataSink audioSink = {&locOutpuMix, nullptr};

    /*
     * create audio player:
     *     fast audio does not support when SL_IID_EFFECTSEND is required, skip it
     *     for fast audio case
     */
    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_VOLUME, SL_IID_EFFECTSEND};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
    result = (*mAudioEngine->engine)->CreateAudioPlayer(mAudioEngine->engine, &mPlayerObj,
                                                        &audioSrc, &audioSink,
                                                        mSampleRate ? 2 : 3, ids, req);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("CreateAudioPlayer failed: %d", result);
        return false;
    }

    result = (*mPlayerObj)->Realize(mPlayerObj, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("mPlayerObj Realize failed: %d", result);
        return false;
    }

    result = (*mPlayerObj)->GetInterface(mPlayerObj, SL_IID_PLAY, &mPlayer);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("mPlayerObj GetInterface failed: %d", result);
        return false;
    }

    result = (*mPlayerObj)->GetInterface(mPlayerObj, SL_IID_BUFFERQUEUE, &mBufferQueue);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("mPlayerObj GetInterface failed: %d", result);
        return false;
    }

    result = (*mBufferQueue)->RegisterCallback(mBufferQueue, playerCallback, this);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("mPlayerObj RegisterCallback failed: %d", result);
        return false;
    }

    mEffectSend = nullptr;
    if (mSampleRate == 0) {
        result = (*mPlayerObj)->GetInterface(mPlayerObj, SL_IID_EFFECTSEND, &mEffectSend);
        if (result != SL_RESULT_SUCCESS) {
            LOGE("mPlayerObj GetInterface failed: %d", result);
            return false;
        }
    }

    result = (*mPlayerObj)->GetInterface(mPlayerObj, SL_IID_VOLUME, &mVolume);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("mPlayerObj GetInterface failed: %d", result);
        return false;
    }

    result = (*mPlayer)->SetPlayState(mPlayer, SL_PLAYSTATE_PLAYING);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("mPlayerObj SetPlayState failed: %d", result);
        return false;
    }

    return true;
}

// 一帧音频播放完毕后就会回调这个函数
void playerCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    BQAudioPlayer *player = (BQAudioPlayer *) context;
    assert(bq == player->mBufferQueue);
    pthread_mutex_unlock(&player->mMutex);
}

void BQAudioPlayer::enqueueSample(void *data, size_t length) {
    // 必须等待一帧音频播放完毕后才可以 Enqueue 第二帧音频
    pthread_mutex_lock(&mMutex);
    if (mBufSize < length) {
        mBufSize = length;
        if (mBuffers[0]) {
            delete[] mBuffers[0];
        }
        if (mBuffers[1]) {
            delete[] mBuffers[1];
        }
        mBuffers[0] = new uint8_t[mBufSize];
        mBuffers[1] = new uint8_t[mBufSize];
    }
    memcpy(mBuffers[mIndex], data, length);
    (*mBufferQueue)->Enqueue(mBufferQueue, mBuffers[mIndex], length);
    mIndex = 1 - mIndex;
}

void BQAudioPlayer::release() {
    pthread_mutex_lock(&mMutex);
    if (mPlayerObj) {
        (*mPlayerObj)->Destroy(mPlayerObj);
        mPlayerObj = nullptr;
        mPlayer = nullptr;
        mBufferQueue = nullptr;
        mEffectSend = nullptr;
        mVolume = nullptr;
    }

    if (mAudioEngine) {
        delete mAudioEngine;
        mAudioEngine = nullptr;
    }

    if (mBuffers[0]) {
        delete[] mBuffers[0];
        mBuffers[0] = nullptr;
    }

    if (mBuffers[1]) {
        delete[] mBuffers[1];
        mBuffers[1] = nullptr;
    }

    pthread_mutex_unlock(&mMutex);
    pthread_mutex_destroy(&mMutex);
}
