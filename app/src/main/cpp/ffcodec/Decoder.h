//
// Created by Administrator on 2018/3/2 0002.
//

#ifndef AVGRAPHICS_DECODER_H
#define AVGRAPHICS_DECODER_H

#include <cstdint>
#include "ffheader.h"
#include "../common/AVModel.h"

class Decoder {
private:
    const char *srcFilePath;
    FILE *pcmFile;
    FILE *yuvFile;
    int width;
    int height;
    int videoStreamIdx;
    int audioStreamIdx;
    int frameRate;
    int sampleRate;
    int channels;
    AVPixelFormat pixelFormat;
    AVSampleFormat sampleFormat;

    uint8_t *videoBuffer[4];
    int videoLinesize[4];
    int videoBufferSize;

    AVFormatContext *formatCtx;
    AVCodecContext *videoCodecCtx;
    AVCodecContext *audioCodecCtx;
    AVStream *videoStream;
    AVStream *audioStream;
    AVFrame *frame;
    AVPacket packet;

    void reset();

    int doDecode(const char *srcFilePath);

    int openInputFile();

    int openCodecCtx(int *streamIdx, AVCodecContext **codecCtx, AVFormatContext *formatContext,
                     AVMediaType mediaType);

    int decodePacket(AVPacket packet, int *gotFrame);

    int release();

public:
    Decoder();

    int decode(const char *srcFilePath, const char *dstVideoPath, const char *dstAudioPath);
};


#endif //AVGRAPHICS_DECODER_H
