//
// Created by zzh on 2018/8/3 0003.
//

#ifndef AVGRAPHICS_YUV_H
#define AVGRAPHICS_YUV_H

#include <cstdint>

class Yuv {
public:
    uint8_t *bufY;
    uint8_t *bufU;
    uint8_t *bufV;

    int strideY;
    int strideU;
    int strideV;

    int width;
    int height;

    Yuv();

    Yuv(int width, int height);

    ~Yuv();

    void alloc(int width, int height);

    Yuv* clone();

    void setData(uint8_t *data);

    void release();
};


#endif //AVGRAPHICS_YUV_H
