//
// Created by Administrator on 2018/3/22 0022.
//

#include "FrameFilter.h"
#include <string>
#include <android/log.h>
#include "result.h"

#define LOG_TAG "ffcodec_filter"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define FLOGE(fmt, ...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, fmt, __VA_ARGS__)

#define DEFAULT_SAMPLE_SIZE 1024

static void androidLog(void *ptr, int level, const char *fmt, va_list vl) {
    if (level == AV_LOG_ERROR || level == AV_LOG_FATAL) {
        FLOGE(fmt, vl);
    }
}

FrameFilter::Parameter::Parameter() {
    width = 0;
    height = 0;
    frameRate = 0;
    pixelFormat = AV_PIX_FMT_NONE;
    outPixelFormat = AV_PIX_FMT_NONE;
    videoFilter = "null";

    sampleRate = 0;
    sampleFormat = AV_SAMPLE_FMT_NONE;
    channels = 0;
    outSampleRate = 0;
    outSampleFormat = AV_SAMPLE_FMT_NONE;
    outChannels = 0;
    audioFilter = "anull";

    flag = FLAG_NONE;
}

void FrameFilter::Parameter::setVideoParams(int width, int height, AVPixelFormat pixelFormat,
                                            int frameRate, const char *videoFilter) {
    this->width = width;
    this->height = height;
    this->frameRate = frameRate;
    this->pixelFormat = pixelFormat;
    if (outPixelFormat == AV_PIX_FMT_NONE) {
        outPixelFormat = pixelFormat;
    }
    this->videoFilter = videoFilter == nullptr ? "null" : videoFilter;
    flag |= FLAG_VIDEO;
}

void FrameFilter::Parameter::setAudioParams(int sampleRate, AVSampleFormat sampleFormat,
                                            int channels, const char *audioFilter) {
    this->sampleRate = sampleRate;
    this->sampleFormat = sampleFormat;
    this->channels = channels;
    if (outChannels == 0) {
        outChannels = channels;
    }
    if (outSampleFormat == AV_SAMPLE_FMT_NONE) {
        outSampleFormat = sampleFormat;
    }
    if (outSampleRate == 0) {
        outSampleRate = sampleRate;
    }
    this->audioFilter = audioFilter == nullptr ? "anull" : audioFilter;
    flag |= FLAG_AUDIO;
}

void FrameFilter::Parameter::setOutputPixelFormat(AVPixelFormat outPixelFormat) {
    this->outPixelFormat = outPixelFormat;
}

void FrameFilter::Parameter::setOutputSampleFormat(AVSampleFormat outSampleFormat) {
    this->outSampleFormat = outSampleFormat;
}

void FrameFilter::Parameter::setOutputSampleRate(int outSampleRate) {
    this->outSampleRate = outSampleRate;
}

void FrameFilter::Parameter::setOutputChannels(int outChannels) {
    this->outChannels = outChannels;
}

std::string FrameFilter::Parameter::toString() {
    char description[1024];
    sprintf(description,
            "{\n[width: %d], [height: %d], [frame rate: %d], [pixel format in: %s, out: %s]\n"
                    "[video filter: %s]\n[sample format in: %s, out: %s], [sample rate in: %d, out: %d], "
                    "[channels in: %d, out: %d]\n[audio filter: %s]\n}",
            width, height, frameRate, av_get_pix_fmt_name(pixelFormat),
            av_get_pix_fmt_name(outPixelFormat), videoFilter, av_get_sample_fmt_name(sampleFormat),
            av_get_sample_fmt_name(outSampleFormat), sampleRate, outSampleRate, channels,
            outChannels, audioFilter);
    std::string str = description;
    return str;
}

FrameFilter::FrameFilter() {
    vbuffersrcCtx = nullptr;
    vbuffersinkCtx = nullptr;
    vfilterGraph = nullptr;

    abuffersrcCtx = nullptr;
    abuffersinkCtx = nullptr;
    afilterGraph = nullptr;

    params = nullptr;
    logEnable = true;
}

int FrameFilter::init(Parameter *params) {
    width = params->width;
    height = params->height;
    inPixelFormat = params->pixelFormat;
    outPixelFormat = params->outPixelFormat;
    frameRate = params->frameRate;
    videoFilter = params->videoFilter;

    inSampleRate = params->sampleRate;
    inSampleFormat = params->sampleFormat;
    inChannels = params->channels;
    outSampleRate = params->outSampleRate;
    outSampleFormat = params->outSampleFormat;
    outChannels = params->outChannels;
    audioFilter = params->audioFilter;

    this->params = params;

    av_register_all();
    avfilter_register_all();
    av_log_set_callback(androidLog);

    if (params->flag & params->FLAG_VIDEO) {
        int ret = initVideo();
        if (ret < 0) {
            LOGE("init video failed!");
            return ret;
        }
    }

    if (params->flag & params->FLAG_AUDIO) {
        int ret = initAudio();
        if (ret < 0) {
            LOGE("init video failed!");
            return ret;
        }
    }

    if (logEnable) {
        LOGI("frame filter init succeed: %s", params->toString().c_str());
    }

    return SUCCEED;
}

int FrameFilter::initVideo() {
    int ret = 0;
    AVRational timebase = av_inv_q(av_d2q(frameRate, 1000000));
    AVRational ratio = av_d2q(1, 255);

    char args[512];
    AVFilter *buffersrc = nullptr;
    AVFilter *buffersink = nullptr;
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs = avfilter_inout_alloc();
    vfilterGraph = avfilter_graph_alloc();

    if (!outputs || !inputs || !vfilterGraph) {
        LOGE("Error allocate video filter object");
        goto end;
    }

    buffersrc = avfilter_get_by_name("buffer");
    buffersink = avfilter_get_by_name("buffersink");
    if (!buffersrc || !buffersink) {
        LOGE("filtering source or sink element not found");
        ret = AVERROR_UNKNOWN;
        goto end;
    }

    // Buffer video frames, and make them available to the filter chain.
    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             width, height, inPixelFormat, timebase.num, timebase.den, ratio.num,
             ratio.den);

    ret = avfilter_graph_create_filter(&vbuffersrcCtx, buffersrc, "in",
                                       args, nullptr, vfilterGraph);
    if (ret < 0) {
        LOGE("Cannot create video buffer source");
        goto end;
    }

    ret = avfilter_graph_create_filter(&vbuffersinkCtx, buffersink, "out",
                                       nullptr, nullptr, vfilterGraph);
    if (ret < 0) {
        LOGE("Cannot create video buffer sink");
        goto end;
    }

    // 用于转换 pixel format
    ret = av_opt_set_bin(vbuffersinkCtx, "pix_fmts",
                         (uint8_t *) &outPixelFormat, sizeof(outPixelFormat),
                         AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        LOGE("Cannot set output pixel format");
        goto end;
    }

    // Endpoints for the filter graph.
    outputs->name = av_strdup("in");
    outputs->filter_ctx = vbuffersrcCtx;
    outputs->pad_idx = 0;
    outputs->next = nullptr;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = vbuffersinkCtx;
    inputs->pad_idx = 0;
    inputs->next = nullptr;

    if (!outputs->name || !inputs->name) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    if ((ret = avfilter_graph_parse_ptr(vfilterGraph, videoFilter,
                                        &inputs, &outputs, nullptr)) < 0)
        goto end;

    if ((ret = avfilter_graph_config(vfilterGraph, nullptr)) < 0)
        goto end;

    end:
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);

    return ret;
}

int FrameFilter::initAudio() {
    int ret = 0;
    AVRational timebase = av_inv_q(av_d2q(inSampleRate, 1000000));

    char args[512];
    AVFilter *buffersrc = nullptr;
    AVFilter *buffersink = nullptr;
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs = avfilter_inout_alloc();
    afilterGraph = avfilter_graph_alloc();
    int64_t outChannelLayout = av_get_default_channel_layout(outChannels);

    if (!outputs || !inputs || !afilterGraph) {
        LOGE("Error allocate audio filter object");
        goto end;
    }

    buffersrc = avfilter_get_by_name("abuffer");
    buffersink = avfilter_get_by_name("abuffersink");
    if (!buffersrc || !buffersink) {
        LOGE("filtering source or sink element not found");
        ret = AVERROR_UNKNOWN;
        goto end;
    }

    // Buffer video frames, and make them available to the filter chain.
    snprintf(args, sizeof(args),
             "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=%lld",
             timebase.num, timebase.den, inSampleRate, av_get_sample_fmt_name(inSampleFormat),
             av_get_default_channel_layout(inChannels));

    ret = avfilter_graph_create_filter(&abuffersrcCtx, buffersrc, "in",
                                       args, nullptr, afilterGraph);
    if (ret < 0) {
        LOGE("Cannot create audio buffer source");
        goto end;
    }

    ret = avfilter_graph_create_filter(&abuffersinkCtx, buffersink, "out",
                                       nullptr, nullptr, afilterGraph);
    if (ret < 0) {
        LOGE("Cannot create audio buffer sink");
        goto end;
    }

    // 转换属性
    if (outSampleFormat != AV_SAMPLE_FMT_NONE) {
        ret = av_opt_set_bin(abuffersinkCtx, "sample_fmts", (uint8_t *) &outSampleFormat,
                             sizeof(outSampleFormat),
                             AV_OPT_SEARCH_CHILDREN);
        if (ret < 0) {
            LOGE("Cannot set output sample format");
            goto end;
        }
    }

    ret = av_opt_set_bin(abuffersinkCtx, "channel_layouts", (uint8_t *) &outChannelLayout,
                         sizeof(outChannelLayout), AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        LOGE("Cannot set output channel layout");
        goto end;
    }

    ret = av_opt_set_bin(abuffersinkCtx, "sample_rates", (uint8_t *) &outSampleRate,
                         sizeof(outSampleRate), AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        LOGE("Cannot set output sample rate");
        goto end;
    }

    // Endpoints for the filter graph.
    outputs->name = av_strdup("in");
    outputs->filter_ctx = abuffersrcCtx;
    outputs->pad_idx = 0;
    outputs->next = nullptr;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = abuffersinkCtx;
    inputs->pad_idx = 0;
    inputs->next = nullptr;

    if (!outputs->name || !inputs->name) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    if ((ret = avfilter_graph_parse_ptr(afilterGraph, audioFilter,
                                        &inputs, &outputs, nullptr)) < 0)
        goto end;

    if ((ret = avfilter_graph_config(afilterGraph, nullptr)) < 0)
        goto end;

    end:
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);

    return ret;
}

int FrameFilter::process(AVModel *model) {
    if (model->flag == MODEL_FLAG_VIDEO) {
        return processImage(model);
    } else if (model->flag == MODEL_FLAG_AUDIO) {
        return processSample(model);
    }

    LOGE("cannnot process such model: %s", model->getName());
    return FAILED;
}

int FrameFilter::processImage(AVModel *model) {
    if (!(params->flag & params->FLAG_VIDEO)) {
        LOGE("audio filter doest not init!");
        return FAILED;
    }

    int ret;
    AVFrame *srcFrame = av_frame_alloc();
    if (!srcFrame) {
        LOGE("Error allocate src frame");
        return FAILED;
    }

    ret = av_image_fill_arrays(srcFrame->data, srcFrame->linesize, model->image, inPixelFormat,
                               width, height, 1);
    if (ret < 0) {
        LOGE("av_image_fill_arrays error: %s", av_err2str(ret));
        return ret;
    }
    srcFrame->width = width;
    srcFrame->height = height;
    srcFrame->format = inPixelFormat;

    ret = av_buffersrc_add_frame_flags(vbuffersrcCtx, srcFrame, 0);
    if (ret < 0) {
        LOGE("Error while feeding the video filtergraph: %s", av_err2str(ret));
        av_frame_free(&srcFrame);
        return ret;
    }

    AVFrame *filtFrame = av_frame_alloc();
    if (!filtFrame) {
        LOGE("Error allocate filt frame");
        av_frame_free(&srcFrame);
        return FAILED;
    }

    ret = av_buffersink_get_frame(vbuffersinkCtx, filtFrame);
    if (ret < 0) {
        LOGE("Error filt frame: %s", av_err2str(ret));
        av_frame_free(&srcFrame);
        av_frame_free(&filtFrame);
        return ret;
    }

    int size = av_image_get_buffer_size(AVPixelFormat(filtFrame->format), filtFrame->width,
                                        filtFrame->height, 1);
    if (size < 0) {
        LOGE("get image buffer size error: %s", av_err2str(size));
        av_frame_free(&srcFrame);
        av_frame_free(&filtFrame);
        return FAILED;
    }
    uint8_t *imageBuf = (uint8_t *) av_malloc((size_t) size);
    if (imageBuf == nullptr) {
        LOGE("Could not allocate memory");
        av_frame_free(&srcFrame);
        av_frame_free(&filtFrame);
        return FAILED;
    }
    ret = av_image_copy_to_buffer(imageBuf, size, (const uint8_t **) filtFrame->data,
                                  filtFrame->linesize, AVPixelFormat(filtFrame->format),
                                  filtFrame->width, filtFrame->height, 1);
    if (ret < 0) {
        LOGE("frame data copy to image buffer failed: %s", av_err2str(ret));
        av_free(imageBuf);
        av_frame_free(&srcFrame);
        av_frame_free(&filtFrame);
        return ret;
    }
    model->freeImage();
    model->image = imageBuf;
    model->imageLen = size;
    model->width = filtFrame->width;
    model->height = filtFrame->height;
    model->pixelFormat = AVPixelFormat(filtFrame->format);

    av_frame_free(&srcFrame);
    av_frame_free(&filtFrame);

    return SUCCEED;
}

int FrameFilter::processSample(AVModel *model) {
    if (!(params->flag & params->FLAG_AUDIO)) {
        LOGE("audio filter doest not init!");
        return FAILED;
    }

    int ret;
    AVFrame *srcFrame = av_frame_alloc();
    if (!srcFrame) {
        LOGE("Error allocate src frame");
        return FAILED;
    }

    ret = av_samples_fill_arrays(srcFrame->data, srcFrame->linesize, model->sample, inChannels,
                                 DEFAULT_SAMPLE_SIZE, inSampleFormat, 1);
    if (ret < 0) {
        LOGE("av_samples_fill_arrays error: %s", av_err2str(ret));
        av_frame_free(&srcFrame);
        return ret;
    }
    // src frame 的参数和 buffersrcCtx 的参数必须相同
    srcFrame->sample_rate = inSampleRate;
    srcFrame->channel_layout = (uint64_t) av_get_default_channel_layout(inChannels);
    srcFrame->channels = inChannels;
    srcFrame->format = inSampleFormat;
    srcFrame->nb_samples = DEFAULT_SAMPLE_SIZE;

    ret = av_buffersrc_add_frame_flags(abuffersrcCtx, srcFrame, 0);
    if (ret < 0) {
        LOGE("Error while feeding the audio filtergraph: %s", av_err2str(ret));
        av_frame_free(&srcFrame);
        return ret;
    }

    AVFrame *filtFrame = av_frame_alloc();
    if (!filtFrame) {
        LOGE("Error allocate filt frame");
        av_frame_free(&srcFrame);
        return FAILED;
    }

    ret = av_buffersink_get_frame(abuffersinkCtx, filtFrame);
    if (ret < 0) {
        LOGE("Error filt frame: %s", av_err2str(ret));
        av_frame_free(&srcFrame);
        av_frame_free(&filtFrame);
        return FAILED;
    }

    int size = av_samples_get_buffer_size(filtFrame->linesize, filtFrame->channels,
                                          filtFrame->nb_samples, AVSampleFormat(filtFrame->format),
                                          1);
    if (size < 0) {
        LOGE("get sample buffer size error: %s", av_err2str(size));
        av_frame_free(&srcFrame);
        av_frame_free(&filtFrame);
        return FAILED;
    }
    uint8_t *sampleBuf = (uint8_t *) av_malloc((size_t) size);
    if (sampleBuf == nullptr) {
        LOGE("Could not allocate memory");
        av_frame_free(&srcFrame);
        av_frame_free(&filtFrame);
        return FAILED;
    }
    ret = av_samples_copy(&sampleBuf, filtFrame->data, 0, 0, filtFrame->nb_samples,
                          filtFrame->channels, AVSampleFormat(filtFrame->format));
    if (ret < 0) {
        LOGE("av_samples_copy error: %s", av_err2str(ret));
        av_free(sampleBuf);
        av_frame_free(&srcFrame);
        av_frame_free(&filtFrame);
        return ret;
    }
    model->freeSample();
    model->sample = sampleBuf;
    model->sampleLen = size;

    av_frame_free(&srcFrame);
    av_frame_free(&filtFrame);

    return SUCCEED;
}

void FrameFilter::reset() {
    vbuffersrcCtx = nullptr;
    vbuffersinkCtx = nullptr;
    if (vfilterGraph != nullptr) {
        avfilter_graph_free(&vfilterGraph);
        vfilterGraph = nullptr;
    }
    abuffersrcCtx = nullptr;
    abuffersinkCtx = nullptr;
    if (afilterGraph != nullptr) {
        avfilter_graph_free(&afilterGraph);
        afilterGraph = nullptr;
    }
    if (params != nullptr) {
        delete params;
        params = nullptr;
    }
}

void FrameFilter::setLogEnable(bool enable) {
    logEnable = enable;
}

FrameFilter::~FrameFilter() {
    reset();
    if (logEnable) {
        LOGI("FrameFilter released");
    }
}
