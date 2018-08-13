//
// Created by zzh on 2018/8/6 0006.
//

#ifndef AVGRAPHICS_PIXEL_FORMAT_H
#define AVGRAPHICS_PIXEL_FORMAT_H

#include <libyuv.h>
#include "Yuv.h"
#include "AVModel.h"

static const int PIXEL_FORMAT_NONE = 0;

static const int PIXEL_FORMAT_NV12 = 1;
static const int PIXEL_FORMAT_NV21 = 2;
static const int PIXEL_FORMAT_YV12 = 3;
static const int PIXEL_FORMAT_YUV420P = 4;

static const int PIXEL_FORMAT_ARGB = 5;
static const int PIXEL_FORMAT_ABGR = 6;

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

Yuv* convertToI420(AVModel *model);

#endif //AVGRAPHICS_PIXEL_FORMAT_H
