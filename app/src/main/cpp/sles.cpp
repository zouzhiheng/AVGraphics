#include <jni.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <assert.h>
#include <pthread.h>
#include <sys/types.h>
#include <malloc.h>
#include <android/log.h>

#define LOG_TAG "opensles"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// engine interfaces
static SLObjectItf engineObj = NULL;
static SLEngineItf engine = NULL;

// output mix interfaces
static SLObjectItf outputMixObj = NULL;
static SLEnvironmentalReverbItf outputMixER = NULL;

// buffer queue player interfaces
static SLObjectItf bqPlayerObj = NULL;
static SLPlayItf bqPlayer = NULL;
static SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;
static SLEffectSendItf bqPlayerEffectSend;
static SLVolumeItf bqPlayerVolume;
static SLmilliHertz bqPlayerSampleRate = 0;
static jint bqPlayerBufSize = 0;
static short *resampleBuf = NULL;

// pointer and size of the next player buffer to enqueue, and number of remaining buffers
static short *nextBuffer;
static unsigned nextSize;
static int nextCount;

// file descriptor player interfaces
static SLObjectItf fdPlayerObj = NULL;
static SLPlayItf fdPlayer;
static SLSeekItf fdPlayerSeek;
static SLMuteSoloItf fdPlayerMuteSolo;
static SLVolumeItf fdPlayerVolume;

// audio recorder interfaces
static SLObjectItf recorderObj = NULL;
static SLRecordItf recorder;
static SLAndroidSimpleBufferQueueItf recorderBQ;


#define RECORDER_FRAMES 4096
static short emptyBuffer[RECORDER_FRAMES];
static short dataBuffer[RECORDER_FRAMES];
static short *recorderBuffers[2] = {emptyBuffer, dataBuffer};
static int recordBufferIndex = 0;
static unsigned recorderSize = RECORDER_FRAMES;
static bool isRecording = false;

FILE *pcmFile = NULL;

static pthread_mutex_t audioEngineLock = PTHREAD_MUTEX_INITIALIZER;

// aux effect on the output mix, used by the buffer queue player
static const SLEnvironmentalReverbSettings reverbSettings =
        SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;


void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context);

void bqRecorderCallback(SLAndroidSimpleBufferQueueItf bq, void *context);

void releaseResampleBuf(void);

void releaseBQPlayer();

void releaseFDPlayer();

void releaseRecorder();

void release();


extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_OpenSLActivity__1createEngine(JNIEnv *env, jclass type) {
    SLresult result;

    // create engine
    result = slCreateEngine(&engineObj, 0, NULL, 0, NULL, NULL);
    assert(result == SL_RESULT_SUCCESS);
    (void) result;

    // realize the engine
    result = (*engineObj)->Realize(engineObj, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    // get the engine interface, which is needed in order to create other objects
    result = (*engineObj)->GetInterface(engineObj, SL_IID_ENGINE, &engine);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    // create output mix, with environmental reverb specified as a non-required interface
    const SLInterfaceID ids[1] = {SL_IID_ENVIRONMENTALREVERB};
    const SLboolean req[1] = {SL_BOOLEAN_FALSE};
    (*engine)->CreateOutputMix(engine, &outputMixObj, 1, ids, req);

    result = (*outputMixObj)->Realize(outputMixObj, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    // get the environmental reverb interface
    // this could fail if the environmental reverb effect is not available,
    // either because the feature is not present, excessive CPU load, or
    // the required MODIFY_AUDIO_SETTINGS permission was not requested and granted
    result = (*outputMixObj)->GetInterface(outputMixObj, SL_IID_ENVIRONMENTALREVERB, &outputMixER);
    if (result == SL_RESULT_SUCCESS) {
        result = (*outputMixER)->SetEnvironmentalReverbProperties(outputMixER, &reverbSettings);
        (void) result;
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_OpenSLActivity__1createBufferQueueAudioPlayer(JNIEnv *env,
                                                                                  jclass type,
                                                                                  jint sampleRate,
                                                                                  jint bufSize) {
    SLresult result;

    if (sampleRate > 0 && bufSize > 0) {
        bqPlayerSampleRate = (SLmilliHertz) (sampleRate * 1000);
        bqPlayerBufSize = bufSize;
    }

    // configure audio source
    SLDataLocator_AndroidSimpleBufferQueue locBufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    SLDataFormat_PCM formatPcm = {SL_DATAFORMAT_PCM, 1, SL_SAMPLINGRATE_8,
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
    SLDataLocator_OutputMix locOutpuMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObj};
    SLDataSink audioSink = {&locOutpuMix, NULL};

    /*
     * create audio player:
     *     fast audio does not support when SL_IID_EFFECTSEND is required, skip it
     *     for fast audio case
     */
    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_VOLUME, SL_IID_EFFECTSEND};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
    result = (*engine)->CreateAudioPlayer(engine, &bqPlayerObj, &audioSource,
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
                                                      NULL);
    assert(result == SL_RESULT_SUCCESS);
    (void) result;

    bqPlayerEffectSend = NULL;
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
}

// this callback handler is called every time a buffer finishes playing
void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    assert(bqPlayerBufferQueue == bq);
    assert(context == NULL);

    // for streaming playback, replace this test by logic to find and fill the next buffer
    if (--nextCount > 0 && nextBuffer != NULL && nextSize != 0) {
        SLresult result;
        // enqueue another buffer
        result = (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, nextBuffer, nextSize);
        if (result != SL_RESULT_SUCCESS) {
            pthread_mutex_unlock(&audioEngineLock);
        }
        (void) result;
    } else {
        releaseResampleBuf();
        pthread_mutex_unlock(&audioEngineLock);
    }

}

void releaseResampleBuf(void) {
    if (bqPlayerSampleRate == 0) {
        /*
         * we are not using fast path, so we were not creating buffers, nothing to do
         */
        return;
    }

    free(resampleBuf);
    resampleBuf = NULL;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_steven_avgraphics_activity_OpenSLActivity__1createAssetAudioPlayer(JNIEnv *env,
                                                                            jclass type,
                                                                            jobject assetManager,
                                                                            jstring filename_) {
    releaseRecorder();

    const char *filename = env->GetStringUTFChars(filename_, 0);
    assert(filename != NULL);

    SLresult result;
    AAssetManager *manager = AAssetManager_fromJava(env, assetManager);
    assert(manager != NULL);
    AAsset *asset = AAssetManager_open(manager, filename, AASSET_MODE_UNKNOWN);

    env->ReleaseStringUTFChars(filename_, filename);

    if (asset == NULL) {
        return JNI_FALSE;
    }

    off_t start, length;
    int fd = AAsset_openFileDescriptor(asset, &start, &length);
    assert(fd >= 0);
    AAsset_close(asset);

    SLDataLocator_AndroidFD locFD = {SL_DATALOCATOR_ANDROIDFD, fd, start, length};
    SLDataFormat_MIME formatMime = {SL_DATAFORMAT_MIME, NULL, SL_CONTAINERTYPE_UNSPECIFIED};
    SLDataSource audioSrc = {&locFD, &formatMime};

    SLDataLocator_OutputMix locOutputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObj};
    SLDataSink audioSink = {&locOutputMix, NULL};

    const SLInterfaceID ids[3] = {SL_IID_SEEK, SL_IID_MUTESOLO, SL_IID_VOLUME};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
    result = (*engine)->CreateAudioPlayer(engine, &fdPlayerObj, &audioSrc, &audioSink,
                                          3, ids, req);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    result = (*fdPlayerObj)->Realize(fdPlayerObj, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    result = (*fdPlayerObj)->GetInterface(fdPlayerObj, SL_IID_PLAY, &fdPlayer);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    result = (*fdPlayerObj)->GetInterface(fdPlayerObj, SL_IID_SEEK, &fdPlayerSeek);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    result = (*fdPlayerObj)->GetInterface(fdPlayerObj, SL_IID_MUTESOLO, &fdPlayerMuteSolo);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    result = (*fdPlayerObj)->GetInterface(fdPlayerObj, SL_IID_VOLUME, &fdPlayerVolume);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    result = (*fdPlayerSeek)->SetLoop(fdPlayerSeek, SL_BOOLEAN_TRUE, 0, SL_TIME_UNKNOWN);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    return JNI_TRUE;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_OpenSLActivity__1setAssetAudioPlayerState(JNIEnv *env,
                                                                              jclass type,
                                                                              jboolean isPlaying) {
    SLresult result;

    if (fdPlayer != NULL) {
        result = (*fdPlayer)->SetPlayState(fdPlayer,
                                           isPlaying ? SL_PLAYSTATE_PLAYING : SL_PLAYSTATE_PAUSED);
        assert(SL_RESULT_SUCCESS == result);
        (void) result;
    }
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_steven_avgraphics_activity_OpenSLActivity__1createAudioRecorder(JNIEnv *env, jclass type,
                                                                         jstring filePath_) {
    const char *filePath = env->GetStringUTFChars(filePath_, 0);

    pcmFile = fopen(filePath, "w");

    SLresult result;

    // deviceType: SL_IODEVICE_AUDIOINPUT
    // deviceId: SL_DEFAULTDEVICEID_AUDIOINPUT
    SLDataLocator_IODevice locDevice = {SL_DATALOCATOR_IODEVICE, SL_IODEVICE_AUDIOINPUT,
                                        SL_DEFAULTDEVICEID_AUDIOINPUT, NULL};
    SLDataSource audioSrc = {&locDevice, NULL};

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
    result = (*engine)->CreateAudioRecorder(engine, &recorderObj, &audioSrc, &audioSink,
                                            1, id, req);
    if (SL_RESULT_SUCCESS != result) {
        return JNI_FALSE;
    }

    result = (*recorderObj)->Realize(recorderObj, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        return JNI_FALSE;
    }

    result = (*recorderObj)->GetInterface(recorderObj, SL_IID_RECORD, &recorder);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    result = (*recorderObj)->GetInterface(recorderObj, SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
                                          &recorderBQ);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    result = (*recorderBQ)->RegisterCallback(recorderBQ, bqRecorderCallback, NULL);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    env->ReleaseStringUTFChars(filePath_, filePath);

    return JNI_TRUE;
}

void bqRecorderCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    assert(bq == recorderBQ);
    assert(NULL == context);

    fwrite(recorderBuffers[recordBufferIndex], 1, recorderSize, pcmFile);
    if (isRecording) {
        recordBufferIndex = 1 - recordBufferIndex;
        (*recorderBQ)->Enqueue(recorderBQ, recorderBuffers[recordBufferIndex], recorderSize);
    } else {
        (*recorder)->SetRecordState(recorder, SL_RECORDSTATE_STOPPED);
        fclose(pcmFile);
        pcmFile = NULL;
        pthread_mutex_unlock(&audioEngineLock);
        LOGI("recorder stopped");
    }

}

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_OpenSLActivity__1startRecord(JNIEnv *env, jclass type) {
    SLresult result;

    if (pthread_mutex_lock(&audioEngineLock)) {
        LOGE("pthread_mutex_lock error");
        return;
    }

    result = (*recorder)->SetRecordState(recorder, SL_RECORDSTATE_STOPPED);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    result = (*recorderBQ)->Clear(recorderBQ);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    result = (*recorderBQ)->Enqueue(recorderBQ, recorderBuffers[recordBufferIndex],
                                    RECORDER_FRAMES * sizeof(short));
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    result = (*recorder)->SetRecordState(recorder, SL_RECORDSTATE_RECORDING);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    isRecording = true;
    LOGI("start recording...");
}

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_OpenSLActivity__1stopRecord(JNIEnv *env, jclass type) {
    isRecording = false;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_OpenSLActivity__1release(JNIEnv *env, jclass type) {
    release();
}

void release() {
    releaseBQPlayer();
    releaseFDPlayer();
    releaseRecorder();

    if (outputMixObj != NULL) {
        (*outputMixObj)->Destroy(outputMixObj);
        outputMixObj = NULL;
        outputMixER = NULL;
    }

    if (engineObj != NULL) {
        (*engineObj)->Destroy(engineObj);
        engineObj = NULL;
        engine = NULL;
    }

    pthread_mutex_destroy(&audioEngineLock);
}

void releaseBQPlayer() {
    if (bqPlayerObj != NULL) {
        (*bqPlayerObj)->Destroy(bqPlayerObj);
        bqPlayerObj = NULL;
        bqPlayer = NULL;
        bqPlayerBufferQueue = NULL;
        bqPlayerEffectSend = NULL;
        bqPlayerVolume = NULL;
    }
}

void releaseFDPlayer() {
    if (fdPlayerObj != NULL) {
        (*fdPlayerObj)->Destroy(fdPlayerObj);
        fdPlayerObj = NULL;
        fdPlayer = NULL;
        fdPlayerSeek = NULL;
        fdPlayerMuteSolo = NULL;
        fdPlayerVolume = NULL;
    }
}

void releaseRecorder() {
    if (recorderObj != NULL) {
        (*recorderObj)->Destroy(recorderObj);
        recorderObj = NULL;
        recorder = NULL;
        recorderBQ = NULL;
    }
}

