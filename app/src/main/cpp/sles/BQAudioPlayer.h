//
// Created by zzh on 2018/7/27 0027.
//

#ifndef AVGRAPHICS_BQAUDIOPLAYER_H
#define AVGRAPHICS_BQAUDIOPLAYER_H

#include <SLES/OpenSLES.h>
#include <stdio.h>
#include <SLES/OpenSLES_Android.h>
#include "AudioEngine.h"

#define SAMPLE_FORMAT_8 8
#define SAMPLE_FORMAT_16 16

class BQAudioPlayer {
private:
    AudioEngine *mAudioEngine;
    SLObjectItf mPlayerObj;
    SLPlayItf mPlayer;
    SLAndroidSimpleBufferQueueItf mBufferQueue;
    SLEffectSendItf mEffectSend;
    SLVolumeItf mVolume;
    SLmilliHertz mSampleRate;
    int mSampleFormat;
    int mChannels;

    uint8_t *mBuffers[2];
    SLuint32 mBufSize;
    int mIndex;
    pthread_mutex_t mMutex;

public:
    BQAudioPlayer(int sampleRate, int sampleFormat, int channels);

    bool init();

    void enqueueSample(void *data, size_t length);

    void release();

    ~BQAudioPlayer();

    friend void playerCallback(SLAndroidSimpleBufferQueueItf bq, void *context);
};

#endif //AVGRAPHICS_BQAUDIOPLAYER_H
