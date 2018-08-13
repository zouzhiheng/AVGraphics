//
// Created by zzh on 2018/3/2 0002.
//

#ifndef AVGRAPHICS_FFHEADER_H
#define AVGRAPHICS_FFHEADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/avfiltergraph.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavdevice/avdevice.h"
#include "libavutil/avutil.h"
#include "libavutil/error.h"
#include "libavutil/samplefmt.h"
#include "libavutil/pixfmt.h"
#include "libavutil/frame.h"
#include "libavutil/imgutils.h"
#include "libavutil/timestamp.h"
#include "libavutil/opt.h"
#include "libavutil/pixdesc.h"
#include "libavutil/mathematics.h"
#include "libavutil/audio_fifo.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"

#ifdef __cplusplus
}
#endif

#endif //AVGRAPHICS_FFHEADER_H
