//
// Created by Administrator on 2018/7/27 0027.
//

#ifndef AVGRAPHICS_BQAUDIOPLAYER_H
#define AVGRAPHICS_BQAUDIOPLAYER_H

#include <SLES/OpenSLES.h>
#include <stdio.h>
#include <SLES/OpenSLES_Android.h>
#include "AudioEngine.h"

class BQAudioPlayer {
private:
    FILE *mFile;

    AudioEngine *mAudioEngine;
    SLObjectItf mPlayerObj;
    SLPlayItf mPlayer;
    SLAndroidSimpleBufferQueueItf mBufferQueue;
    SLEffectSendItf mEffectSend;
    SLVolumeItf mVolume;
    SLmilliHertz mSampleRate;

    short *mBuffers[2];
    SLuint32 mBufSize;
    int mIndex;
    bool mIsPlaying;

    pthread_mutex_t mMutex;
    pthread_t mPlayThread;

private:
    void initPlayer(SLmilliHertz sampleRate, SLuint32 bufSize);

    void release();

public:
    BQAudioPlayer(const char *filePath);

    void start();

    void stop();

    ~BQAudioPlayer();

    friend void *playThread(void *arg);

    friend void playerCallback(SLAndroidSimpleBufferQueueItf bq, void *context);
};

#endif //AVGRAPHICS_BQAUDIOPLAYER_H
