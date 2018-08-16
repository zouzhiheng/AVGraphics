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
        int channels;
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
    const char *mDstFilePath;
    int mWidth;
    int mHeight;
    int mFrameRate;
    int mSampleRate;
    int mChannels;
    AVPixelFormat mPixelFormat;
    AVSampleFormat mSampleFormat;
    AVCodecID mVideoCodecID;
    AVCodecID mAudioCodecID;
    int64_t mMaxBitRate;
    bool mFrameRateFixed;
    bool mHaveVideo;
    bool mHaveAudio;
    Parameter *mParams;

    int mImageCount;
    int mSampleCount;
    int64_t mStartPts;
    int64_t mLastPts;

    AVFormatContext *mFormatCtx;
    AVCodecContext *mVideoCodecCtx;
    AVCodecContext *mAudioCodecCtx;
    AVStream *mVideoStream;
    AVStream *mAudioStream;
    AVFrame *mSample;
    uint8_t **mSampleBuf;
    int mSampleBufSize;
    int mSamplePlanes;
    AVFrame *mPicture;
    AVFrame *mTmpPicture;
    uint8_t *mPictureBuf;
    SwsContext *mImgConvertCtx;
    SwrContext *mSampleConvertCtx;

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
