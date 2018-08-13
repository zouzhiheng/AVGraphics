//
// Created by zzh on 2018/3/2 0002.
//

#include "AVModel.h"
#include "format_util.h"

AVModel::AVModel() : image(nullptr), imageLen(0), sample(nullptr), sampleLen(0),
                     width(0), height(0), pixelFormat(PIXEL_FORMAT_NONE), flag(-1) {

}

const char *AVModel::getName() {
    if (flag == MODEL_FLAG_VIDEO) {
        return "video";
    } else if (flag == MODEL_FLAG_AUDIO) {
        return "audio";
    }
    return "unknown";
}

void AVModel::freeImage() {
    if (image != nullptr) {
        delete image;
        image = nullptr;
    }
    imageLen = 0;
}

void AVModel::freeSample() {
    if (sample != nullptr) {
        delete sample;
        sample = nullptr;
    }
    sampleLen = 0;
}

AVModel::~AVModel() {
    freeImage();
    freeSample();
}
