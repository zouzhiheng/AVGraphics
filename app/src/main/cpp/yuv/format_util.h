//
// Created by zzh on 2018/8/2 0002.
//

#ifndef AVGRAPHICS_FORMAT_UTIL_H
#define AVGRAPHICS_FORMAT_UTIL_H

#include <libyuv.h>
#include <pixfmt.h>
#include "Yuv.h"
#include "AVModel.h"

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


#endif //AVGRAPHICS_FORMAT_UTIL_H
