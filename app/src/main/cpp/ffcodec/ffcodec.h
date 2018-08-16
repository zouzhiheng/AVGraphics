//
// Created by zzh on 2018/8/13 0013.
//

#ifndef AVGRAPHICS_FFCODEC_H
#define AVGRAPHICS_FFCODEC_H

#include <cstdint>
#include <jni.h>

class RecordParams {
private:
    const char *dstFile;
    int width;
    int height;
    int frameRate;
    int sampleRate;
    int pixelFormatFlag;
    int sampleFormatFlag;
    int channels;
    int64_t maxBitRate;
    int quality;

    int cropX;
    int cropY;
    int cropW;
    int cropH;
    int rotateDegree;
    int scaleW;
    int scaleH;
    bool mirror;

    const char *videoFilter;
    const char *audioFilter;

    jobject callbackObj;
    jmethodID callbackMethod;

public:
    RecordParams(const char *dstFile, int width, int height, int frameRate, int sampleRate,
                 int pixelFormatFlag, int sampleFormatFlag, int channels);

    void setMaxBitRate(int64_t maxBitRate);

    void setQuality(int quality);

    void setCrop(int cropX, int cropY, int cropW, int cropH);

    void setRotate(int rotateDegree);

    void setScale(int scaleW, int scaleH);

    void setMirror(bool mirror);

    void setVideoFilter(const char *videoFilter);

    void setAudioFilter(const char *audioFilter);

    void setCallback(jobject callbackObj, jmethodID callbackMethod);

    friend int initRecorder(RecordParams *params);

    friend void *recordThreadCallback(void *arg);
};

int initRecorder(RecordParams *params);

int recordImage(uint8_t *image, int imageLen, int width, int height, int pixelFormatFlag);

int recordSample(uint8_t *sample, int sampleLen);

void stopRecord();

#endif //AVGRAPHICS_FFCODEC_H
