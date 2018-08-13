//
// Created by zzh on 2018/4/13 0013.
//

#include "YuvProcessor.h"
#include "log.h"
#include "format.h"
#include <libyuv/convert.h>
#include <malloc.h>
#include <string.h>

using namespace std;
using namespace libyuv;

#define SUCCEED 1
#define FAILED -1

YuvProcessor::Parameter::Parameter(int srcW, int srcH, int pixelFormat)
        : srcW(srcW), srcH(srcH), pixelFormat(pixelFormat), cropX(0), cropY(0), cropW(0), cropH(0),
          rotateMode(kRotate0), scaleW(0), scaleH(0), mirror(false) {

}

void YuvProcessor::Parameter::setCrop(int cropX, int cropY, int cropW, int cropH) {
    this->cropX = cropX;
    this->cropY = cropY;
    this->cropW = cropW;
    this->cropH = cropH;
}

void YuvProcessor::Parameter::setRotate(int degree) {
    switch (degree) {
        case 90:
            rotateMode = kRotate90;
            break;
        case 180:
            rotateMode = kRotate180;
            break;
        case 270:
            rotateMode = kRotate270;
            break;
        default:
            rotateMode = kRotate0;
            break;
    }
}

void YuvProcessor::Parameter::setScale(int scaleW, int scaleH) {
    this->scaleW = scaleW;
    this->scaleH = scaleH;
}

void YuvProcessor::Parameter::setMirror(bool mirror) {
    this->mirror = mirror;
}

std::string YuvProcessor::Parameter::toString() {
    char description[1024];
    sprintf(description, "{\n[cropX: %d], [cropY: %d], [cropW: %d], [cropH: %d]\n[rotate: %d]\n"
                    "[scaleW: %d], [scaleH: %d]\n[mirror: %d]\n}",
            cropX, cropY, cropW, cropH, rotateMode, scaleW, scaleH, mirror);
    string str = description;
    return str;
}

YuvProcessor::YuvProcessor() {
    reset();
}

void YuvProcessor::reset() {
    cropX = 0;
    cropY = 0;
    cropW = 0;
    cropH = 0;
    rotateMode = kRotate0;
    scaleW = 0;
    scaleH = 0;
    mirror = false;

    cropYuv = nullptr;
    scaleYuv = nullptr;
    mirrorYuv = nullptr;

    params = nullptr;
}

int YuvProcessor::init(Parameter *params) {
    this->params = params;
    cropX = params->cropX;
    cropY = params->cropY;
    cropW = params->cropW;
    cropH = params->cropH;
    rotateMode = params->rotateMode;
    scaleW = params->scaleW;
    scaleH = params->scaleH;
    mirror = params->mirror;

    if ((cropW == 0 || cropH == 0) && rotateMode == kRotate0 && scaleW == 0 && scaleH == 0 &&
        !mirror && params->pixelFormat == PIXEL_FORMAT_YUV420P) {
        return FAILED;
    }

    if (cropW == 0 && cropH == 0) {
        cropW = params->srcW;
        cropH = params->srcH;
    }

    if (cropW % 2 == 1) {
        if (cropH >= cropW) {
            cropH = (int) (1.0f * (cropW - 1) / cropW * cropH);
            cropH = cropH % 2 == 1 ? cropH - 1 : cropH;
        }
        cropW--;
    }

    if (cropH % 2 == 1) {
        if (cropW >= cropH) {
            cropW = (int) (1.0f * (cropH - 1) / cropH * cropW);
            cropW = cropW % 2 == 1 ? cropW - 1 : cropW;
        }
        cropH--;
    }

    if (cropW > 0 && cropH > 0) {
        int width = rotateMode == kRotate0 || rotateMode == kRotate180 ? cropW : cropH;
        int height = rotateMode == kRotate0 || rotateMode == kRotate180 ? cropH : cropW;
        cropYuv = new Yuv();
        cropYuv->alloc(width, height);
    }

    if (scaleW > 0 && scaleH > 0) {
        scaleYuv = new Yuv();
        scaleYuv->alloc(scaleW, scaleH);
    }

    if (mirror) {
        mirrorYuv = new Yuv();
        mirrorYuv->alloc(getOutputWidth(), getOutputHeight());
    }

    params->cropW = cropW;
    params->cropH = cropH;
    LOGI("yuv process params: %s", params->toString().c_str());

    return SUCCEED;
}

int YuvProcessor::process(AVModel *model) {
    if (model->flag != MODEL_FLAG_VIDEO) {
        LOGE("cannot process such model: %s", model->getName());
        return FAILED;
    }

    if (cropX + cropW > model->width || cropY + cropH > model->height) {
        LOGE("crop argument invalid, model: [%d, %d], crop: [%d, %d, %d, %d]", model->width,
             model->height, cropX, cropY, cropW, cropH);
        return FAILED;
    }

    int ret;
    ret = ConvertToI420(model->image, (size_t) model->imageLen,
                        cropYuv->bufY, cropYuv->strideY, cropYuv->bufU, cropYuv->strideU,
                        cropYuv->bufV, cropYuv->strideV, cropX, cropY,
                        model->width, model->height, cropW, cropH, rotateMode,
                        getFourCC(model->pixelFormat));
    if (ret < 0) {
        LOGE("ConvertToI420 error: %d", ret);
        return ret;
    }
    Yuv *src = cropYuv;
    Yuv *output = cropYuv;
    int outputWidth = rotateMode == kRotate0 || rotateMode == kRotate180 ? cropW : cropH;
    int outputHeight = rotateMode == kRotate0 || rotateMode == kRotate180 ? cropH : cropW;

    if (scaleW > 0 && scaleH > 0) {
        if (processScale(src, outputWidth, outputHeight) < 0) {
            return FAILED;
        }
        src = scaleYuv;
        output = scaleYuv;
        outputWidth = scaleW;
        outputHeight = scaleH;
    }

    if (mirror) {
        if (processMirror(src, outputWidth, outputHeight) < 0) {
            return FAILED;
        }
        output = mirrorYuv;
    }

    fillModel(model, output, outputWidth, outputHeight);

    return SUCCEED;
}

int YuvProcessor::processScale(Yuv *src, int srcW, int srcH) {
    int ret;
    ret = I420Scale(src->bufY, src->strideY, src->bufU, src->strideU, src->bufV, src->strideV,
                    srcW, srcH,
                    scaleYuv->bufY, scaleYuv->strideY, scaleYuv->bufU, scaleYuv->strideU,
                    scaleYuv->bufV, scaleYuv->strideV,
                    scaleW, scaleH,
                    kFilterBox);
    if (ret < 0) {
        LOGE("I420Scale error: %d", ret);
        return ret;
    }

    return SUCCEED;
}

int YuvProcessor::processMirror(Yuv *src, int srcW, int srcH) {
    int ret;
    ret = I420Mirror(src->bufY, src->strideY, src->bufU, src->strideU, src->bufV, src->strideV,
                     mirrorYuv->bufY, mirrorYuv->strideY, mirrorYuv->bufU, mirrorYuv->strideU,
                     mirrorYuv->bufV, mirrorYuv->strideV,
                     srcW, srcH);
    if (ret < 0) {
        LOGE("I420Mirror error: %d", ret);
        return ret;
    }
    return SUCCEED;
}

void YuvProcessor::fillModel(AVModel *model, Yuv *src, int srcW, int srcH) {
    uint8_t *image = new uint8_t[srcW * srcH * 3 / 2];
    if (model != nullptr) {
        model->freeImage();
    } else {
        model = new AVModel();
    }
    model->image = image;
    memcpy(model->image, src->bufY, (size_t) srcW * srcH);
    memcpy(model->image + srcW * srcH, src->bufU, (size_t) srcW * srcH / 4);
    memcpy(model->image + srcW * srcH * 5 / 4, src->bufV, (size_t) srcW * srcH / 4);
    model->imageLen = srcW * srcH * 3 / 2;
    model->width = srcW;
    model->height = srcH;
    model->pixelFormat = PIXEL_FORMAT_YUV420P;
}

int YuvProcessor::getOutputWidth() {
    int outputWidth = scaleW;
    if (outputWidth == 0) {
        outputWidth = rotateMode == kRotate0 || rotateMode == kRotate180 ? cropW : cropH;
    }
    return outputWidth;
}

int YuvProcessor::getOutputHeight() {
    int outputHeight = scaleH;
    if (outputHeight == 0) {
        outputHeight = rotateMode == kRotate0 || rotateMode == kRotate180 ? cropH : cropW;
    }
    return outputHeight;
}

YuvProcessor::~YuvProcessor() {
    if (cropYuv != nullptr) {
        delete cropYuv;
    }

    if (scaleYuv != nullptr) {
        delete scaleYuv;
    }

    if (mirrorYuv != nullptr) {
        delete mirrorYuv;
    }

    if (params != nullptr) {
        delete params;
    }

    reset();
    LOGI("YuvProcessor released");
}