//
// Created by zzh on 2018/3/22 0022.
//

#ifndef AVGRAPHICS_FRAME_FILTER_H
#define AVGRAPHICS_FRAME_FILTER_H

#include <string>
#include "ffheader.h"
#include "AVModel.h"

class FrameFilter {
public:
    class Parameter {
    private:
        int mWidth;
        int mHeight;
        int mFrameRate;
        AVPixelFormat mPixelFormat;
        AVPixelFormat mOutPixelFormat;
        const char *mVideoFilter;

        int mSampleRate;
        AVSampleFormat mSampleFormat;
        int mChannels;
        int mOutSampleRate;
        AVSampleFormat mOutSampleFormat;
        int mOutChannels;
        const char *mAudioFilter;

        static const int FLAG_NONE = 0;
        static const int FLAG_VIDEO = 1;
        static const int FLAG_AUDIO = 2;
        int mFlag = 0;

    public:
        Parameter();

        void setVideoParams(int width, int height, AVPixelFormat pixelFormat, int frameRate,
                            const char *videoFilter);

        void setAudioParams(int sampleRate, AVSampleFormat sampleFormat, int channels,
                            const char *audioFilter);

        void setOutputPixelFormat(AVPixelFormat outPixelFormat);

        void setOutputSampleFormat(AVSampleFormat outSampleFormat);

        void setOutputSampleRate(int outSampleRate);

        void setOutputChannels(int outChannels);

        std::string toString();

        friend class FrameFilter;
    };

private:
    int mWidth;
    int mHeight;
    int mFrameRate;
    AVPixelFormat mInPixelFormat;
    AVPixelFormat mOutPixelFormat;
    const char *mVideoFilter;

    int mInSampleRate;
    AVSampleFormat mInSampleFormat;
    int mInChannels;
    int mOutSampleRate;
    AVSampleFormat mOutSampleFormat;
    int mOutChannels;
    const char *mAudioFilter;

    Parameter *mParams;

    AVFilterContext *mVBuffersinkCtx;
    AVFilterContext *mVBuffersrcCtx;
    AVFilterGraph *mVFfilterGraph;

    AVFilterContext *mABuffersinkCtx;
    AVFilterContext *mABbuffersrcCtx;
    AVFilterGraph *mAFilterGraph;

private:
    int initVideo();

    int initAudio();

    int processImage(AVModel *model);

    int processSample(AVModel *model);

public:
    FrameFilter();

    ~FrameFilter();

    int init(Parameter *params);

    int process(AVModel *model);

    void reset();
};

#endif //AVGRAPHICS_FRAME_FILTER_H
