//
// Created by Administrator on 2018/7/27 0027.
//

#include <assert.h>
#include "BQAudioPlayer.h"
#include "log.h"

void playerCallback(SLAndroidSimpleBufferQueueItf bq, void *context);

BQAudioPlayer::BQAudioPlayer(const char *filePath)
        : mAudioEngine(new AudioEngine()), mFile(fopen(filePath, "r")), mPlayerObj(nullptr), mPlayer(nullptr),
          mBufferQueue(nullptr), mEffectSend(nullptr), mVolume(nullptr), mSampleRate(0),
          mBufSize(0), mResampleBuf(nullptr), mIsPlaying(false) {

}

void BQAudioPlayer::initPlayer(const char *filePath, int sampleRate, int bufSize) {
    SLresult result;

    if (sampleRate > 0 && bufSize > 0) {
        mSampleRate = (SLmilliHertz) (sampleRate * 1000);
        mBufSize = bufSize;
    }
    LOGI("sample rate: %d, buf size: %d, bq sample rate: %d, bq buf size: %d", sampleRate, bufSize,
         mSampleRate, mBufSize);

    SLDataLocator_AndroidSimpleBufferQueue locBufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    SLDataFormat_PCM formatPcm = {SL_DATAFORMAT_PCM, 1, SL_SAMPLINGRATE_48,
                                  SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
                                  SL_SPEAKER_FRONT_CENTER, SL_BYTEORDER_LITTLEENDIAN};
    /*
     * Enable Fast Audio when possible:  once we set the same rate to be the native, fast audio path
     * will be triggered
     */
    if (mSampleRate) {
        formatPcm.samplesPerSec = mSampleRate;
    }
    SLDataSource audioSource = {&locBufq, &formatPcm};

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
                                                        &audioSource, &audioSink,
                                                        mSampleRate ? 2 : 3, ids, req);
    assert(result == SL_RESULT_SUCCESS);
    (void) result;

    result = (*mPlayerObj)->Realize(mPlayerObj, SL_BOOLEAN_FALSE);
    assert(result == SL_RESULT_SUCCESS);
    (void) result;

    result = (*mPlayerObj)->GetInterface(mPlayerObj, SL_IID_PLAY, &mPlayer);
    assert(result == SL_RESULT_SUCCESS);
    (void) result;

    result = (*mPlayerObj)->GetInterface(mPlayerObj, SL_IID_BUFFERQUEUE, &mBufferQueue);
    assert(result == SL_RESULT_SUCCESS);
    (void) result;

    result = (*mBufferQueue)->RegisterCallback(mBufferQueue, playerCallback, nullptr);
    assert(result == SL_RESULT_SUCCESS);
    (void) result;

    mEffectSend = nullptr;
    if (mSampleRate == 0) {
        result = (*mPlayerObj)->GetInterface(mPlayerObj, SL_IID_EFFECTSEND, &mEffectSend);
        assert(result == SL_RESULT_SUCCESS);
        (void) result;
    }

    result = (*mPlayerObj)->GetInterface(mPlayerObj, SL_IID_VOLUME, &mVolume);
    assert(result == SL_RESULT_SUCCESS);
    (void) result;

    result = (*mPlayer)->SetPlayState(mPlayer, SL_PLAYSTATE_PLAYING);
    assert(result == SL_RESULT_SUCCESS);
    (void) result;

    mIsPlaying = true;
}

void playerCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {

}

BQAudioPlayer::~BQAudioPlayer() {
    release();
}

void BQAudioPlayer::release() {
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

    if (mResampleBuf) {
        delete mResampleBuf;
        mResampleBuf = nullptr;
    }

    if (mFile) {
        fclose(mFile);
        mFile = nullptr;
    }
}
