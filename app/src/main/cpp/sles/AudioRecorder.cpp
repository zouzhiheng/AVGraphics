//
// Created by zzh on 2018/7/27 0027.
//

#include "AudioRecorder.h"
#include "log.h"

#define RECORDER_FRAMES 2048

AudioRecorder::AudioRecorder(const char *filePath)
        : mAudioEngine(new AudioEngine()), mFile(fopen(filePath, "w")), mRecorderObj(nullptr),
          mRecorder(nullptr), mBufferQueue(nullptr), mBufSize(RECORDER_FRAMES), mIndex(0),
          mIsRecording(false), mIsInitialized(false) {
    mBuffers[1] = new short[mBufSize];
    mBuffers[0] = new short[mBufSize];
}

bool AudioRecorder::start() {
    if (!mIsInitialized) {
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

    result = (*mBufferQueue)->Clear(mBufferQueue);
    if (SL_RESULT_SUCCESS != result) {
        return false;
    }

    // enqueue an empty buffer to be filled by the recorder
    // (for streaming recording, we would enqueue at least 2 empty buffers to start things off)
    result = (*mBufferQueue)->Enqueue(mBufferQueue, mBuffers[mIndex], mBufSize * sizeof(short));
    if (SL_RESULT_SUCCESS != result) {
        return false;
    }

    // 开始录制
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
                                           &mBufferQueue);
    if (SL_RESULT_SUCCESS != result) {
        return false;
    }

    result = (*mBufferQueue)->RegisterCallback(mBufferQueue, recorderCallback, this);
    return SL_RESULT_SUCCESS == result;
}

// 每录制完成一帧音频，就会回调这个函数
void AudioRecorder::recorderCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    assert(context != nullptr);

    AudioRecorder *recorder = (AudioRecorder *) context;
    assert(bq == recorder->mBufferQueue);

    fwrite(recorder->mBuffers[recorder->mIndex], 1, recorder->mBufSize,
           recorder->mFile);
    if (recorder->mIsRecording) {
        recorder->mIndex = 1 - recorder->mIndex;
        (*recorder->mBufferQueue)->Enqueue(recorder->mBufferQueue,
                                           recorder->mBuffers[recorder->mIndex],
                                           recorder->mBufSize);
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
        mBufferQueue = nullptr;
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

    if (mFile) {
        fclose(mFile);
        mFile = nullptr;
    }

    mIsInitialized = false;
    mIsRecording = false;
    mIndex = 0;
    LOGI("AudioRecorder stopped");
}
