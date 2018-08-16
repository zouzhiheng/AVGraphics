//
// Created by zzh on 2018/3/5 0005.
//

#ifndef AVGRAPHICS_TRANSCODER_H
#define AVGRAPHICS_TRANSCODER_H

#include "ffheader.h"
#include <string>
#include <map>
#include <vector>

class Transcoder {
public:
    class Options {
    private:
        uint64_t start;
        uint64_t duration;
        const char *videoFilter;
        const char *audioFilter;
        int64_t maxBitRate;
        bool reencode;
        std::map<std::string, std::string> encodeOptions;

    public:
        Options();

        void setCutTime(uint64_t start, uint64_t duration);

        void setVideoFilter(const char *videoFilter);

        void setAudioFilter(const char *audioFilter);

        void setMaxBitRate(int64_t maxBitRate);

        void setQuality(int quality);

        void addEncodeOptions(std::string key, std::string value);

        void setReencode(bool reencode);

        friend class Transcoder;
    };

private:
    typedef struct FilteringContext {
        AVFilterContext *buffersinkCtx;
        AVFilterContext *buffersrcCtx;
        AVFilterGraph *filterGraph;
    } FilteringContext;

    typedef struct StreamContext {
        AVCodecContext *decodeCtx;
        AVCodecContext *encodeCtx;
    } StreamContext;

private:
    Options *options;
    int64_t videoPts;
    int64_t audioPts;
    int64_t audioPtsDelay;
    int totalFrame;
    int viStreamIndex; // video in stream index
    int aoStreamIndex; // audio out stream index

    AVFormatContext *ifmtCtx;
    AVFormatContext *ofmtCtx;
    FilteringContext *filterCtx;
    StreamContext *streamCtx;

    AVFrame *tmpFrame;
    uint8_t *tmpFrameBuf;
    SwsContext *imgConvertCtx;

    AVAudioFifo *audioFifo;
    SwrContext *sampleConvertCtx;

private:
    void reset();

    int64_t getNearlyIFramePts(const char *filePath, int64_t targetPts);

    int64_t getNearlyIFramePts(const char *filePath, int64_t targetPts, bool backward);

    int seekFrame(AVFormatContext *formatCtx, AVStream *stream, int64_t timestamp);

    int openInputFile(const char *filename);

    int openOutputFile(const char *filename);

    int initFilters();

    int initFilters(FilteringContext *filterCtx, AVCodecContext *decodeCtx,
                    AVCodecContext *encodeCtx, const char *filterSpec);

    int handleEncodeWriteFrame(AVFrame *frame, int streamIndex);

    int filterEncodeVideo(AVFrame *frame, int streamIndex);

    int encodeAudioWidthFifo(AVFrame *frame, int streamIndex);

    int storeAudioFrame(AVFrame *inputFrame, AVCodecContext *outCodecCtx);

    int initAudioOutputFrame(AVFrame **frame, AVCodecContext *outCodecCtx, int frameSize);

    int readAudioToEncode(int streamIndex);

    int encodeAudioOrVideo(AVFrame *frame, int streamIndex);

    int rescaleVideoToEncode(AVFrame *frame, int streamIndex);

    int doEncodeWriteFrame(AVFrame *frame, int streamIndex, int *gotFrame);

    int flushEncoder(int streamIndex);

public:
    Transcoder();

    int transcode(const char *srcFilePath, const char *dstFilePath, Options *options_);

    int transcode(const char *srcFilePath, const char *dstFilePath, int64_t start,
                  int64_t duration);
};

#endif //AVGRAPHICS_TRANSCODER_H
