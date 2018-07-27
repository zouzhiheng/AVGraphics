//
// Created by Administrator on 2018/7/27 0027.
//

#ifndef AVGRAPHICS_BQAUDIOPLAYER_H
#define AVGRAPHICS_BQAUDIOPLAYER_H

#include <SLES/OpenSLES.h>
#include <stdio.h>
#include <SLES/OpenSLES_Android.h>
#include "AudioEngine.h"

class BQAudioPlayer : public AudioEngine {
private:
    FILE *mFile;

    SLObjectItf mPlayerObj;
    SLPlayItf mPlayer;
    SLAndroidSimpleBufferQueueItf mBufferQueue;
    SLEffectSendItf mEffectSend;
    SLVolumeItf mVolume;
    SLmilliHertz mSampleRate;
    jint mBufSize;
    short *mResampleBuf;

    bool mIsPlaying;

private:
    void initPlayer(const char *filePath, int sampleRate, int bufSize);

protected:
    void release() override;

public:
    BQAudioPlayer(const char *filePath);

    virtual ~BQAudioPlayer();
};

#endif //AVGRAPHICS_BQAUDIOPLAYER_H
