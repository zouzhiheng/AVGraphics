//
// Created by Administrator on 2018/3/29 0029.
//

#ifndef VIDEOEDITOR_AVINFO_H
#define VIDEOEDITOR_AVINFO_H

#include <cstdint>

extern "C" {
#include <libavutil/samplefmt.h>
#include <libavcodec/avcodec.h>
};

class AVInfo {
public:
    int width;
    int height;
    int64_t duration;
    int frameRate;
    int sampleRate;
    AVPixelFormat pixelFormat;
    AVSampleFormat sampleFormat;
    AVCodecID videoCodecID;
    AVCodecID audioCodecID;
    int64_t bitRate;
    int channels;
    bool haveVideo;
    bool haveAudio;

    AVInfo(): width(0), height(0), duration(0), frameRate(0),
              pixelFormat(AV_PIX_FMT_NONE), sampleRate(0), sampleFormat(AV_SAMPLE_FMT_NONE),
              videoCodecID(AV_CODEC_ID_NONE), audioCodecID(AV_CODEC_ID_NONE), bitRate(0),
              channels(0), haveVideo(false), haveAudio(false) {

    }
};

#endif //VIDEOEDITOR_AVINFO_H
