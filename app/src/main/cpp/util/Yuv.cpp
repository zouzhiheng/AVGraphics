//
// Created by Administrator on 2018/8/3 0003.
//

#include <malloc.h>
#include <cstring>
#include "Yuv.h"
#include "log.h"


Yuv::Yuv() {
    bufY = nullptr;
    bufU = nullptr;
    bufV = nullptr;
    strideY = 0;
    strideU = 0;
    strideV = 0;
}

Yuv::~Yuv() {
    release();
}

void Yuv::alloc(int width, int height) {
    strideY = width;
    strideU = width / 2;
    strideV = width / 2;
    bufY = (uint8_t *) malloc((size_t) width * height);
    bufU = (uint8_t *) malloc((size_t) width * height / 4);
    bufV = (uint8_t *) malloc((size_t) width * height / 4);
    this->width = width;
    this->height = height;
}

void Yuv::setData(uint8_t *data) {
    memcpy(bufY, data, (size_t) (width * height));
    memcpy(bufU, data + width * height, (size_t) (width * height / 4));
    memcpy(bufV, data + width * height * 5 / 4, (size_t) (width * height / 4));
}

Yuv *Yuv::clone() {
    if (width <= 0 || height <= 0) {
        return nullptr;
    }
    Yuv *yuv = new Yuv();
    yuv->alloc(width, height);
    memcpy(yuv->bufY, bufY, (size_t) (width * height));
    memcpy(yuv->bufU, bufU, (size_t) (width * height / 4));
    memcpy(yuv->bufV, bufV, (size_t) (width * height / 4));
    return yuv;
}

void Yuv::release() {
    if (bufY != nullptr) {
        free(bufY);
        bufY = nullptr;
    }
    strideY = 0;

    if (bufU != nullptr) {
        free(bufU);
        bufU = nullptr;
    }
    strideU = 0;

    if (bufV != nullptr) {
        free(bufV);
        bufV = nullptr;
    }
    strideV = 0;
}
