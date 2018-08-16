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
        int width;
        int height;
        int frameRate;
        AVPixelFormat pixelFormat;
        AVPixelFormat outPixelFormat;
        const char *videoFilter;

        int sampleRate;
        AVSampleFormat sampleFormat;
        int channels;
        int outSampleRate;
        AVSampleFormat outSampleFormat;
        int outChannels;
        const char *audioFilter;

        static const int FLAG_NONE = 0;
        static const int FLAG_VIDEO = 1;
        static const int FLAG_AUDIO = 2;
        int flag = 0;

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
    int width;
    int height;
    int frameRate;
    AVPixelFormat inPixelFormat;
    AVPixelFormat outPixelFormat;
    const char *videoFilter;

    int inSampleRate;
    AVSampleFormat inSampleFormat;
    int inChannels;
    int outSampleRate;
    AVSampleFormat outSampleFormat;
    int outChannels;
    const char *audioFilter;

    Parameter *params;

    AVFilterContext *vbuffersinkCtx;
    AVFilterContext *vbuffersrcCtx;
    AVFilterGraph *vfilterGraph;

    AVFilterContext *abuffersinkCtx;
    AVFilterContext *abuffersrcCtx;
    AVFilterGraph *afilterGraph;

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
