//
// Created by Administrator on 2018/7/27 0027.
//

#include <assert.h>
#include "BQAudioPlayer.h"
#include "log.h"

void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context);

BQAudioPlayer::BQAudioPlayer(FILE *mFile) : AudioEngine(), mFile(mFile) {

}

BQAudioPlayer::~BQAudioPlayer() {

}

void BQAudioPlayer::initPlayer(const char* filePath, int sampleRate, int bufSize) {
    if (mFile) {
        fclose(mFile);
    }
    mFile = fopen(filePath, "r");

    SLresult result;

    if (sampleRate > 0 && bufSize > 0) {
        bqPlayerSampleRate = (SLmilliHertz) (sampleRate * 1000);
        bqPlayerBufSize = bufSize;
    }
    LOGI("sample rate: %d, buf size: %d, bq sample rate: %d, bq buf size: %d", sampleRate, bufSize,
         bqPlayerSampleRate, bqPlayerBufSize);

    // configure audio source
    SLDataLocator_AndroidSimpleBufferQueue locBufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    SLDataFormat_PCM formatPcm = {SL_DATAFORMAT_PCM, 1, SL_SAMPLINGRATE_48,
                                  SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
                                  SL_SPEAKER_FRONT_CENTER, SL_BYTEORDER_LITTLEENDIAN};
    /*
     * Enable Fast Audio when possible:  once we set the same rate to be the native, fast audio path
     * will be triggered
     */
    if (bqPlayerSampleRate) {
        formatPcm.samplesPerSec = bqPlayerSampleRate;
    }
    SLDataSource audioSource = {&locBufq, &formatPcm};

    // configure audio sink
    SLDataLocator_OutputMix locOutpuMix = {SL_DATALOCATOR_OUTPUTMIX, mOutputMixObj};
    SLDataSink audioSink = {&locOutpuMix, nullptr};

    /*
     * create audio player:
     *     fast audio does not support when SL_IID_EFFECTSEND is required, skip it
     *     for fast audio case
     */
    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_VOLUME, SL_IID_EFFECTSEND};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
    result = (*mEngine)->CreateAudioPlayer(mEngine, &bqPlayerObj, &audioSource,
                                          &audioSink, bqPlayerSampleRate ? 2 : 3, ids, req);
    assert(result == SL_RESULT_SUCCESS);
    (void) result;

    result = (*bqPlayerObj)->Realize(bqPlayerObj, SL_BOOLEAN_FALSE);
    assert(result == SL_RESULT_SUCCESS);
    (void) result;

    result = (*bqPlayerObj)->GetInterface(bqPlayerObj, SL_IID_PLAY, &bqPlayer);
    assert(result == SL_RESULT_SUCCESS);
    (void) result;

    result = (*bqPlayerObj)->GetInterface(bqPlayerObj, SL_IID_BUFFERQUEUE, &bqPlayerBufferQueue);
    assert(result == SL_RESULT_SUCCESS);
    (void) result;

    result = (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback,
                                                      nullptr);
    assert(result == SL_RESULT_SUCCESS);
    (void) result;

    bqPlayerEffectSend = nullptr;
    if (bqPlayerSampleRate == 0) {
        result = (*bqPlayerObj)->GetInterface(bqPlayerObj, SL_IID_EFFECTSEND, &bqPlayerEffectSend);
        assert(result == SL_RESULT_SUCCESS);
        (void) result;
    }

    result = (*bqPlayerObj)->GetInterface(bqPlayerObj, SL_IID_VOLUME, &bqPlayerVolume);
    assert(result == SL_RESULT_SUCCESS);
    (void) result;

    result = (*bqPlayer)->SetPlayState(bqPlayer, SL_PLAYSTATE_PLAYING);
    assert(result == SL_RESULT_SUCCESS);
    (void) result;

    isPlaying = true;
}

void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {

}
