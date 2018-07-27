//
// Created by Administrator on 2018/7/27 0027.
//

#include "AudioRecorder.h"
#include "log.h"

#define RECORDER_FRAMES 960

AudioRecorder::AudioRecorder(const char *filePath)
        : mAudioEngine(new AudioEngine()), mFile(fopen(filePath, "w")), mRecorderObj(nullptr),
          mRecorder(nullptr), mRecorderBQ(nullptr), mRecorderBufSize(RECORDER_FRAMES), mIndex(0),
          mIsRecording(false), mIsInitialized(false) {
    mRecorderBuffers[1] = new short[mRecorderBufSize];
    mRecorderBuffers[0] = new short[mRecorderBufSize];
}

bool AudioRecorder::start() {
    if (!mIsInitialized) {
        mAudioEngine->createEngine();
        if (!initRecorder()) {
            return false;
        }
        mIsInitialized = true;
    }

    SLresult result;

    result = (*mRecorder)->SetRecordState(mRecorder, SL_RECORDSTATE_STOPPED);
    if (SL_RESULT_SUCCESS != result) {
        return false;
    }

    result = (*mRecorderBQ)->Clear(mRecorderBQ);
    if (SL_RESULT_SUCCESS != result) {
        return false;
    }

    result = (*mRecorderBQ)->Enqueue(mRecorderBQ, mRecorderBuffers[mIndex],
                                     mRecorderBufSize * sizeof(short));
    if (SL_RESULT_SUCCESS != result) {
        return false;
    }

    result = (*mRecorder)->SetRecordState(mRecorder, SL_RECORDSTATE_RECORDING);
    if (SL_RESULT_SUCCESS != result) {
        return false;
    }

    mIsRecording = true;
    LOGI("start recording...");

    return true;
}

bool AudioRecorder::initRecorder() {
    SLresult result;

    SLDataLocator_IODevice locDevice = {SL_DATALOCATOR_IODEVICE, SL_IODEVICE_AUDIOINPUT,
                                        SL_DEFAULTDEVICEID_AUDIOINPUT, nullptr};
    SLDataSource audioSrc = {&locDevice, nullptr};

    // num buffers: 2
    SLDataLocator_AndroidSimpleBufferQueue locBQ = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    // format: PCM
    // channel: 1
    // sampleRate: 48000
    // sampleFormat: 16bit
    // endian: little endian
    SLDataFormat_PCM formatPcm = {SL_DATAFORMAT_PCM, 1, SL_SAMPLINGRATE_48,
                                  SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
                                  SL_SPEAKER_FRONT_CENTER, SL_BYTEORDER_LITTLEENDIAN};
    SLDataSink audioSink = {&locBQ, &formatPcm};

    const SLInterfaceID id[1] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};
    result = (*mAudioEngine->engine)->CreateAudioRecorder(mAudioEngine->engine, &mRecorderObj,
                                                          &audioSrc, &audioSink, 1, id, req);
    if (SL_RESULT_SUCCESS != result) {
        return false;
    }

    result = (*mRecorderObj)->Realize(mRecorderObj, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        return false;
    }

    result = (*mRecorderObj)->GetInterface(mRecorderObj, SL_IID_RECORD, &mRecorder);
    if (SL_RESULT_SUCCESS != result) {
        return false;
    }

    result = (*mRecorderObj)->GetInterface(mRecorderObj, SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
                                           &mRecorderBQ);
    if (SL_RESULT_SUCCESS != result) {
        return false;
    }

    result = (*mRecorderBQ)->RegisterCallback(mRecorderBQ, recorderCallback, this);
    return SL_RESULT_SUCCESS == result;

}

void AudioRecorder::recorderCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    assert(context != nullptr);

    AudioRecorder *recorder = (AudioRecorder *) context;
    assert(bq == recorder->mRecorderBQ);

    fwrite(recorder->mRecorderBuffers[recorder->mIndex], 1, recorder->mRecorderBufSize,
           recorder->mFile);
    if (recorder->mIsRecording) {
        recorder->mIndex = 1 - recorder->mIndex;
        (*recorder->mRecorderBQ)->Enqueue(recorder->mRecorderBQ,
                                          recorder->mRecorderBuffers[recorder->mIndex],
                                          recorder->mRecorderBufSize);
    } else {
        (*recorder->mRecorder)->SetRecordState(recorder->mRecorder, SL_RECORDSTATE_STOPPED);
        fclose(recorder->mFile);
        recorder->mFile = nullptr;
    }
}

void AudioRecorder::stop() {
    mIsRecording = false;
}

AudioRecorder::~AudioRecorder() {
    release();
}

void AudioRecorder::release() {
    if (mRecorderObj) {
        (*mRecorder)->SetRecordState(mRecorder, SL_RECORDSTATE_STOPPED);
        (*mRecorderObj)->Destroy(mRecorderObj);
        mRecorderObj = nullptr;
        mRecorder = nullptr;
        mRecorderBQ = nullptr;
    }

    if (mAudioEngine) {
        mAudioEngine->release();
        delete mAudioEngine;
        mAudioEngine = nullptr;
    }

    if (mRecorderBuffers[0]) {
        delete[] mRecorderBuffers[0];
        mRecorderBuffers[0] = nullptr;
    }

    if (mRecorderBuffers[1]) {
        delete[] mRecorderBuffers[1];
        mRecorderBuffers[1] = nullptr;
    }

    if (mFile) {
        fclose(mFile);
        mFile = nullptr;
    }

    mIsInitialized = false;
    mIsRecording = false;
    mIndex = 0;
    LOGI("AudioRecorder stopped");
}
