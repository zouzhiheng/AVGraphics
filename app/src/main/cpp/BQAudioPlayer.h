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

    SLObjectItf bqPlayerObj;
    SLPlayItf bqPlayer;
    SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;
    SLEffectSendItf bqPlayerEffectSend;
    SLVolumeItf bqPlayerVolume;
    SLmilliHertz bqPlayerSampleRate;
    jint bqPlayerBufSize;
    short *resampleBuf;

    bool isPlaying;

private:
    void initPlayer(const char* filePath, int sampleRate, int bufSize);

public:
    BQAudioPlayer(FILE *mFile);

    virtual ~BQAudioPlayer();
};

#endif //AVGRAPHICS_BQAUDIOPLAYER_H
