//
// Created by Administrator on 2018/7/27 0027.
//

#ifndef AVGRAPHICS_AUDIORECORDER_H
#define AVGRAPHICS_AUDIORECORDER_H

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <stdio.h>
#include "AudioEngine.h"

class AudioRecorder : public AudioEngine {
private:
    FILE *mFile;

    SLObjectItf mRecorderObj;
    SLRecordItf mRecorder;
    SLAndroidSimpleBufferQueueItf mRecorderBQ;

    unsigned mRecorderSize;
    short *mWorkBuffer;
    short *mIdleBuffer;
    short *mRecorderBuffers[2];
    int mIndex;

    bool mIsRecording;
    bool mIsInitialized;

private:
    bool initRecorder();

    void release() override;

    static void recorderCallback(SLAndroidSimpleBufferQueueItf bq, void *context);

public:
    AudioRecorder(const char *filePath);

    bool start();

    void stop();

    virtual ~AudioRecorder();
};

#endif //AVGRAPHICS_AUDIORECORDER_H
