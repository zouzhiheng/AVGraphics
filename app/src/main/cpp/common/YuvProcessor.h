//
// Created by zzh on 2018/4/13 0013.
//

#ifndef AVGRAPHICS_YUV_PROCESSER_H
#define AVGRAPHICS_YUV_PROCESSER_H

#include <libyuv.h>
#include <string>
#include "AVModel.h"
#include "Yuv.h"

class YuvProcessor {
public:
    class Parameter {
    private:
        int srcW;
        int srcH;
        int pixelFormat;

        int cropX;
        int cropY;
        int cropW;
        int cropH;
        libyuv::RotationMode rotateMode;
        int scaleW;
        int scaleH;
        bool mirror;

    public:
        Parameter(int srcW, int srcH, int pixelFormat);

        void setCrop(int cropX, int cropY, int cropW, int cropH);

        void setRotate(int degree);

        void setScale(int scaleW, int scaleH);

        void setMirror(bool mirror);

        std::string toString();

        friend class YuvProcessor;
    };

private:
    Parameter *params;

    int cropX;
    int cropY;
    int cropW;
    int cropH;
    libyuv::RotationMode rotateMode;
    int scaleW;
    int scaleH;
    bool mirror;

    Yuv *cropYuv;
    Yuv *scaleYuv;
    Yuv *mirrorYuv;

    void reset();

    int processScale(Yuv *src, int srcW, int srcH);

    int processMirror(Yuv *src,int srcW, int srcH);

    void fillModel(AVModel *model, Yuv *src, int srcW, int srcH);

public:
    YuvProcessor();

    ~YuvProcessor();

    int init(Parameter *params);

    int process(AVModel *model);

    int getOutputWidth();

    int getOutputHeight();
};

#endif //AVGRAPHICS_YUV_PROCESSER_H
