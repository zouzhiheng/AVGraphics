//
// Created by zzh on 2018/8/6 0006.
//

#ifndef AVGRAPHICS_PIXEL_FORMAT_H
#define AVGRAPHICS_PIXEL_FORMAT_H

#include <libyuv.h>
#include <libavutil/samplefmt.h>
#include "Yuv.h"
#include "AVModel.h"

extern "C" {
#include <libavutil/pixfmt.h>
};

static const int VCODEC_UNKNOWN = 0;
static const int VCODEC_H264 = 1;
static const int VCODEC_MPEG = 2;
static const int VCODEC_H265 = 3;
static const int VCODEC_OTHER = 4;

static const int ACODEC_UNKNOWN = 0;
static const int ACODEC_AAC = 1;
static const int ACODEC_MP3 = 2;
static const int ACODEC_OTHER = 3;

static const int PIXEL_FORMAT_NONE = 0;
static const int PIXEL_FORMAT_NV21 = 1;
static const int PIXEL_FORMAT_YV12 = 2;
static const int PIXEL_FORMAT_NV12 = 3;
static const int PIXEL_FORMAT_YUV420P = 4;
static const int PIXEL_FORMAT_ARGB = 5;
static const int PIXEL_FORMAT_ABGR = 6;

static const int SAMPLE_FORMAT_8BIT = 8;
static const int SAMPLE_FORMAT_16BIT = 16;
static const int SAMPLE_FORMAT_FLOAT = 32;

inline libyuv::FourCC getFourCC(int pixelFormat) {
    if (pixelFormat == PIXEL_FORMAT_NV12) {
        return libyuv::FOURCC_NV12;
    } else if (pixelFormat == PIXEL_FORMAT_NV21) {
        return libyuv::FOURCC_NV21;
    } else if (pixelFormat == PIXEL_FORMAT_YV12) {
        return libyuv::FOURCC_YV12;
    } else if (pixelFormat == PIXEL_FORMAT_YUV420P) {
        return libyuv::FOURCC_I420;
    } else if (pixelFormat == PIXEL_FORMAT_ABGR) {
        return libyuv::FOURCC_ABGR;
    } else if (pixelFormat == PIXEL_FORMAT_ARGB) {
        return libyuv::FOURCC_ARGB;
    }
    return libyuv::FOURCC_ANY;
}

inline AVPixelFormat getPixelFormat(int flag) {
    AVPixelFormat pixelFormat = AV_PIX_FMT_NONE;
    if (flag == PIXEL_FORMAT_NV21) {
        pixelFormat = AV_PIX_FMT_NV21;
    } else if (flag == PIXEL_FORMAT_YV12 || flag == PIXEL_FORMAT_YUV420P) {
        pixelFormat = AV_PIX_FMT_YUV420P;
    } else if (flag == PIXEL_FORMAT_ABGR) {
        pixelFormat = AV_PIX_FMT_ABGR;
    }
    return pixelFormat;
}

inline int pixelFormatToInt(AVPixelFormat pixelFormat) {
    int flag = PIXEL_FORMAT_NONE;
    if (pixelFormat == AV_PIX_FMT_NV12) {
        flag = PIXEL_FORMAT_NV12;
    } else if (pixelFormat == AV_PIX_FMT_NV21) {
        flag = PIXEL_FORMAT_NV21;
    } else if (pixelFormat == AV_PIX_FMT_YUV420P) {
        flag = PIXEL_FORMAT_YUV420P;
    }
    return flag;
}

inline AVSampleFormat getSampleFormat(int flag) {
    AVSampleFormat sampleFormat = AV_SAMPLE_FMT_NONE;
    if (flag == SAMPLE_FORMAT_8BIT) {
        sampleFormat = AV_SAMPLE_FMT_U8;
    } else if (flag == SAMPLE_FORMAT_16BIT) {
        sampleFormat = AV_SAMPLE_FMT_S16;
    } else if (flag == SAMPLE_FORMAT_FLOAT) {
        sampleFormat = AV_SAMPLE_FMT_FLT;
    }
    return sampleFormat;
}

Yuv *convertToI420(AVModel *model);

#endif //AVGRAPHICS_PIXEL_FORMAT_H
