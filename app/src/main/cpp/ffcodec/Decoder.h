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
    const char *mSrcFilePath;
    FILE *mPcmFile;
    FILE *mYuvFile;
    int mWidth;
    int mHeight;
    int mVideoStreamIdx;
    int mAudioStreamIdx;
    AVPixelFormat mPixelFormat;

    uint8_t *mVideoBuffers[4];
    int mVideoLinesize[4];
    int mVideoBufferSize;

    AVFormatContext *mFormatCtx;
    AVCodecContext *mVideoCodecCtx;
    AVCodecContext *mAudioCodecCtx;
    AVFrame *mFrame;
    AVPacket mPacket;

    void reset();

    int doDecode(const char *srcFilePath);

    int openInputFile();

    int openCodecCtx(int *streamIdx, AVCodecContext **codecCtx, AVFormatContext *formatContext,
                     AVMediaType mediaType);

    void decodePacket(AVPacket packet, int *gotFrame);

    int release();

public:
    Decoder();

    int decode(const char *srcFilePath, const char *dstVideoPath, const char *dstAudioPath);
};


#endif //AVGRAPHICS_DECODER_H
