//
// Created by zzh on 2018/8/13 0013.
//

#ifndef AVGRAPHICS_DECODER_H
#define AVGRAPHICS_DECODER_H

#include <cstdint>
#include <AVInfo.h>
#include "ffheader.h"
#include "../common/AVModel.h"

class Decoder {
private:
    const char *mSrcFilePath;
    FILE *mPcmFile;
    FILE *mYuvFile;
    int mWidth;
    int mHeight;
    int mFrameRate;
    int mSampleRate;
    int mVideoStreamIdx;
    int mAudioStreamIdx;
    AVPixelFormat mPixelFormat;
    AVSampleFormat mSampleFormat;

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

    AVInfo* getAVInfo(const char *srcFilePath);

    int decode(const char *srcFilePath, const char *dstVideoPath, const char *dstAudioPath);
};


#endif //AVGRAPHICS_DECODER_H
