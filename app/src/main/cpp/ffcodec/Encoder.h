//
// Created by zzh on 2018/8/13 0013.
//

#ifndef AVGRAPHICS_ENCODER_H
#define AVGRAPHICS_ENCODER_H

#include "ffheader.h"
#include "AVModel.h"
#include <string>
#include <map>

class Encoder {
public:
    class Parameter {
    private:
        const char *dstFilePath;
        int width;
        int height;
        int frameRate;
        int sampleRate;
        AVPixelFormat pixelFormat;
        AVSampleFormat sampleFormat;
        int channels; // for audio only
        AVCodecID videoCodecID;
        AVCodecID audioCodecID;
        int64_t maxBitRate;
        bool frameRateFixed;
        bool haveVideo;
        bool haveAudio;
        std::map<std::string, std::string> encodeOptions;
        std::map<std::string, std::string> videoMetadata;

    public:
        Parameter(const char *dstFilePath, int width, int height, int frameRate,
                  int sampleRate, int channels, AVPixelFormat pixelFormat,
                  AVSampleFormat sampleFormat);

        void setVideoCodec(AVCodecID codecID);

        void setAudioCodec(AVCodecID codecID);

        void setMaxBitRate(int64_t maxBitRate);

        void setQuality(int quality);

        void setFrameRateFixed(bool frameRateFixed);

        void setHaveVideo(bool haveVideo);

        void setHaveAudio(bool haveAudio);

        void addEncodeOptions(std::string key, std::string value);

        void addVideoMetadata(std::string key, std::string value);

        std::string toString();

        friend class Encoder;
    };

private:
    const char *dstFilePath;
    int width;
    int height;
    int frameRate;
    int sampleRate;
    int channels; // for audio only
    AVPixelFormat pixelFormat;
    AVSampleFormat sampleFormat;
    AVCodecID videoCodecID;
    AVCodecID audioCodecID;
    int64_t maxBitRate;
    bool frameRateFixed;
    bool haveVideo;
    bool haveAudio;
    Parameter *params;

    int imageCount;
    int sampleCount;
    int64_t startPts;
    int64_t lastPts;

    AVFormatContext *formatCtx;
    AVCodecContext *videoCodecCtx;
    AVCodecContext *audioCodecCtx;
    AVStream *videoStream;
    AVStream *audioStream;
    AVFrame *sample;
    uint8_t **sampleBuf;
    int sampleBufSize;
    int samplePlanes;
    AVFrame *picture;
    AVFrame *tmpPicture;
    uint8_t *pictureBuf;
    SwsContext *imgConvertCtx;
    SwrContext *sampleConvertCtx;

private:
    int openOutputFile();

    int openCodecCtx(AVCodecID codecID, AVMediaType mediaType);

    int fillPicture(AVModel *model);

    int fillSample(AVModel *model);

    void release();

public:
    int init(Parameter *params);

    int encode(AVModel *model);

    int encode(AVModel *model, int *gotFrame);

    int stop();

    ~Encoder();
};

#endif //AVGRAPHICS_ENCODER_H
