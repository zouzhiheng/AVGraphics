//
// Created by zzh on 2018/7/27 0027.
//

#ifndef AVGRAPHICS_AUDIORECORDER_H
#define AVGRAPHICS_AUDIORECORDER_H

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <stdio.h>
#include "AudioEngine.h"

class AudioRecorder {
private:
    FILE *mFile;

    AudioEngine *mAudioEngine;
    SLObjectItf mRecorderObj;
    SLRecordItf mRecorder;
    SLAndroidSimpleBufferQueueItf mBufferQueue;

    unsigned mBufSize;
    short *mBuffers[2];
    int mIndex;

    bool mIsRecording;
    bool mIsInitialized;

private:
    bool initRecorder();

    void release();

    static void recorderCallback(SLAndroidSimpleBufferQueueItf bq, void *context);

public:
    AudioRecorder(const char *filePath);

    bool start();

    void stop();

    virtual ~AudioRecorder();
};

#endif //AVGRAPHICS_AUDIORECORDER_H
