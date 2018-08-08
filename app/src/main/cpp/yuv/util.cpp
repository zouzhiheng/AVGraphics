//
// Created by Administrator on 2018/8/2 0002.
//

#include <sys/time.h>
#include "util.h"
#include "log.h"

using namespace libyuv;

uint64_t getCurrentTimeMs() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    uint64_t us = (uint64_t) (tv.tv_sec) * 1000 * 1000 + (uint64_t) (tv.tv_usec);
    return us / 1000;
}

Yuv* convertToI420(AVModel *model) {
    if (!model || model->imageLen <= 0 || model->flag != MODEL_FLAG_VIDEO || model->width <= 0
        || model->height <= 0 || model->pixelFormat <= 0 || !model->image) {
        LOGE("convertToARGB failed: invalid argument");
        return nullptr;
    }
    Yuv *yuv = new Yuv(model->width, model->height);
    ConvertToI420(model->image, (size_t) model->imageLen, yuv->bufY, yuv->strideY,
                  yuv->bufU, yuv->strideU, yuv->bufV, yuv->strideV,
                  0, 0, model->width, model->height, model->width, model->height,
                  kRotate0, getFourCC(model->pixelFormat));
    return yuv;
}

