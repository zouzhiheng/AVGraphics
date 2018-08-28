//
// Created by zzh on 2018/3/22 0022.
//

#include "FrameFilter.h"
#include <string>
#include <android/log.h>
#include <format.h>
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
    mWidth = 0;
    mHeight = 0;
    mFrameRate = 0;
    mPixelFormat = AV_PIX_FMT_NONE;
    mOutPixelFormat = AV_PIX_FMT_NONE;
    mVideoFilter = "null";

    mSampleRate = 0;
    mSampleFormat = AV_SAMPLE_FMT_NONE;
    mChannels = 0;
    mOutSampleRate = 0;
    mOutSampleFormat = AV_SAMPLE_FMT_NONE;
    mOutChannels = 0;
    mAudioFilter = "anull";

    mFlag = FLAG_NONE;
}

void FrameFilter::Parameter::setVideoParams(int width, int height, AVPixelFormat pixelFormat,
                                            int frameRate, const char *videoFilter) {
    this->mWidth = width;
    this->mHeight = height;
    this->mFrameRate = frameRate;
    this->mPixelFormat = pixelFormat;
    if (mOutPixelFormat == AV_PIX_FMT_NONE) {
        mOutPixelFormat = pixelFormat;
    }
    this->mVideoFilter = videoFilter == nullptr ? "null" : videoFilter;
    mFlag |= FLAG_VIDEO;
}

void FrameFilter::Parameter::setAudioParams(int sampleRate, AVSampleFormat sampleFormat,
                                            int channels, const char *audioFilter) {
    this->mSampleRate = sampleRate;
    this->mSampleFormat = sampleFormat;
    this->mChannels = channels;
    if (mOutChannels == 0) {
        mOutChannels = channels;
    }
    if (mOutSampleFormat == AV_SAMPLE_FMT_NONE) {
        mOutSampleFormat = sampleFormat;
    }
    if (mOutSampleRate == 0) {
        mOutSampleRate = sampleRate;
    }
    this->mAudioFilter = audioFilter == nullptr ? "anull" : audioFilter;
    mFlag |= FLAG_AUDIO;
}

void FrameFilter::Parameter::setOutputPixelFormat(AVPixelFormat outPixelFormat) {
    this->mOutPixelFormat = outPixelFormat;
}

void FrameFilter::Parameter::setOutputSampleFormat(AVSampleFormat outSampleFormat) {
    this->mOutSampleFormat = outSampleFormat;
}

void FrameFilter::Parameter::setOutputSampleRate(int outSampleRate) {
    this->mOutSampleRate = outSampleRate;
}

void FrameFilter::Parameter::setOutputChannels(int outChannels) {
    this->mOutChannels = outChannels;
}

std::string FrameFilter::Parameter::toString() {
    char description[1024];
    sprintf(description,
            "{\n[width: %d], [height: %d], [frame rate: %d], [pixel format in: %s, out: %s]\n"
                    "[video filter: %s]\n[sample format in: %s, out: %s], [sample rate in: %d, out: %d], "
                    "[channels in: %d, out: %d]\n[audio filter: %s]\n}",
            mWidth, mHeight, mFrameRate, av_get_pix_fmt_name(mPixelFormat),
            av_get_pix_fmt_name(mOutPixelFormat), mVideoFilter,
            av_get_sample_fmt_name(mSampleFormat),
            av_get_sample_fmt_name(mOutSampleFormat), mSampleRate, mOutSampleRate, mChannels,
            mOutChannels, mAudioFilter);
    std::string str = description;
    return str;
}

FrameFilter::FrameFilter() {
    mVBuffersrcCtx = nullptr;
    mVBuffersinkCtx = nullptr;
    mVFfilterGraph = nullptr;

    mABbuffersrcCtx = nullptr;
    mABuffersinkCtx = nullptr;
    mAFilterGraph = nullptr;

    mParams = nullptr;
}

int FrameFilter::init(Parameter *params) {
    mWidth = params->mWidth;
    mHeight = params->mHeight;
    mInPixelFormat = params->mPixelFormat;
    mOutPixelFormat = params->mOutPixelFormat;
    mFrameRate = params->mFrameRate;
    mVideoFilter = params->mVideoFilter;

    mInSampleRate = params->mSampleRate;
    mInSampleFormat = params->mSampleFormat;
    mInChannels = params->mChannels;
    mOutSampleRate = params->mOutSampleRate;
    mOutSampleFormat = params->mOutSampleFormat;
    mOutChannels = params->mOutChannels;
    mAudioFilter = params->mAudioFilter;

    this->mParams = params;

    av_register_all();
    avfilter_register_all();
    av_log_set_callback(androidLog);

    if (params->mFlag & params->FLAG_VIDEO) {
        int ret = initVideo();
        if (ret < 0) {
            LOGE("init video failed!");
            return ret;
        }
    }

    if (params->mFlag & params->FLAG_AUDIO) {
        int ret = initAudio();
        if (ret < 0) {
            LOGE("init video failed!");
            return ret;
        }
    }

    return SUCCEED;
}

int FrameFilter::initVideo() {
    int ret = 0;
    AVRational timebase = av_inv_q(av_d2q(mFrameRate, 1000000));
    AVRational ratio = av_d2q(1, 255);

    char args[512];
    AVFilter *buffersrc = nullptr;
    AVFilter *buffersink = nullptr;
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs = avfilter_inout_alloc();
    mVFfilterGraph = avfilter_graph_alloc();

    if (!outputs || !inputs || !mVFfilterGraph) {
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
             mWidth, mHeight, mInPixelFormat, timebase.num, timebase.den, ratio.num,
             ratio.den);

    ret = avfilter_graph_create_filter(&mVBuffersrcCtx, buffersrc, "in",
                                       args, nullptr, mVFfilterGraph);
    if (ret < 0) {
        LOGE("Cannot create video buffer source");
        goto end;
    }

    ret = avfilter_graph_create_filter(&mVBuffersinkCtx, buffersink, "out",
                                       nullptr, nullptr, mVFfilterGraph);
    if (ret < 0) {
        LOGE("Cannot create video buffer sink");
        goto end;
    }

    // 用于转换 pixel format
    ret = av_opt_set_bin(mVBuffersinkCtx, "pix_fmts",
                         (uint8_t *) &mOutPixelFormat, sizeof(mOutPixelFormat),
                         AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        LOGE("Cannot set output pixel format");
        goto end;
    }

    // Endpoints for the filter graph.
    outputs->name = av_strdup("in");
    outputs->filter_ctx = mVBuffersrcCtx;
    outputs->pad_idx = 0;
    outputs->next = nullptr;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = mVBuffersinkCtx;
    inputs->pad_idx = 0;
    inputs->next = nullptr;

    if (!outputs->name || !inputs->name) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    if ((ret = avfilter_graph_parse_ptr(mVFfilterGraph, mVideoFilter,
                                        &inputs, &outputs, nullptr)) < 0)
        goto end;

    if ((ret = avfilter_graph_config(mVFfilterGraph, nullptr)) < 0)
        goto end;

    end:
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);

    return ret;
}

int FrameFilter::initAudio() {
    int ret = 0;
    AVRational timebase = av_inv_q(av_d2q(mInSampleRate, 1000000));

    char args[512];
    AVFilter *buffersrc = nullptr;
    AVFilter *buffersink = nullptr;
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs = avfilter_inout_alloc();
    mAFilterGraph = avfilter_graph_alloc();
    int64_t outChannelLayout = av_get_default_channel_layout(mOutChannels);

    if (!outputs || !inputs || !mAFilterGraph) {
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
             timebase.num, timebase.den, mInSampleRate, av_get_sample_fmt_name(mInSampleFormat),
             av_get_default_channel_layout(mInChannels));

    ret = avfilter_graph_create_filter(&mABbuffersrcCtx, buffersrc, "in",
                                       args, nullptr, mAFilterGraph);
    if (ret < 0) {
        LOGE("Cannot create audio buffer source");
        goto end;
    }

    ret = avfilter_graph_create_filter(&mABuffersinkCtx, buffersink, "out",
                                       nullptr, nullptr, mAFilterGraph);
    if (ret < 0) {
        LOGE("Cannot create audio buffer sink");
        goto end;
    }

    // 转换属性
    if (mOutSampleFormat != AV_SAMPLE_FMT_NONE) {
        ret = av_opt_set_bin(mABuffersinkCtx, "sample_fmts", (uint8_t *) &mOutSampleFormat,
                             sizeof(mOutSampleFormat),
                             AV_OPT_SEARCH_CHILDREN);
        if (ret < 0) {
            LOGE("Cannot set output sample format");
            goto end;
        }
    }

    ret = av_opt_set_bin(mABuffersinkCtx, "channel_layouts", (uint8_t *) &outChannelLayout,
                         sizeof(outChannelLayout), AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        LOGE("Cannot set output channel layout");
        goto end;
    }

    ret = av_opt_set_bin(mABuffersinkCtx, "sample_rates", (uint8_t *) &mOutSampleRate,
                         sizeof(mOutSampleRate), AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        LOGE("Cannot set output sample rate");
        goto end;
    }

    // Endpoints for the filter graph.
    outputs->name = av_strdup("in");
    outputs->filter_ctx = mABbuffersrcCtx;
    outputs->pad_idx = 0;
    outputs->next = nullptr;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = mABuffersinkCtx;
    inputs->pad_idx = 0;
    inputs->next = nullptr;

    if (!outputs->name || !inputs->name) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    if ((ret = avfilter_graph_parse_ptr(mAFilterGraph, mAudioFilter,
                                        &inputs, &outputs, nullptr)) < 0)
        goto end;

    if ((ret = avfilter_graph_config(mAFilterGraph, nullptr)) < 0)
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
    if (!(mParams->mFlag & mParams->FLAG_VIDEO)) {
        LOGE("audio filter doest not init!");
        return FAILED;
    }

    int ret;
    AVFrame *srcFrame = av_frame_alloc();
    if (!srcFrame) {
        LOGE("Error allocate src frame");
        return FAILED;
    }

    ret = av_image_fill_arrays(srcFrame->data, srcFrame->linesize, model->image, mInPixelFormat,
                               mWidth, mHeight, 1);
    if (ret < 0) {
        LOGE("av_image_fill_arrays error: %s, [%d, %d, %s], [%d, %d, %s], [%d, %d, %s]",
             av_err2str(ret), srcFrame->width, srcFrame->height,
             av_get_pix_fmt_name((AVPixelFormat) srcFrame->format), mWidth, mHeight,
             av_get_pix_fmt_name(mInPixelFormat), model->width, model->height,
             av_get_pix_fmt_name(getPixelFormat(model->pixelFormat)));
        return ret;
    }
    srcFrame->width = mWidth;
    srcFrame->height = mHeight;
    srcFrame->format = mInPixelFormat;

    ret = av_buffersrc_add_frame_flags(mVBuffersrcCtx, srcFrame, 0);
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

    ret = av_buffersink_get_frame(mVBuffersinkCtx, filtFrame);
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
    model->pixelFormat = pixelFormatToInt(AVPixelFormat(filtFrame->format));

    av_frame_free(&srcFrame);
    av_frame_free(&filtFrame);

    return SUCCEED;
}

int FrameFilter::processSample(AVModel *model) {
    if (!(mParams->mFlag & mParams->FLAG_AUDIO)) {
        LOGE("audio filter doest not init!");
        return FAILED;
    }

    int ret;
    AVFrame *srcFrame = av_frame_alloc();
    if (!srcFrame) {
        LOGE("Error allocate src frame");
        return FAILED;
    }

    ret = av_samples_fill_arrays(srcFrame->data, srcFrame->linesize, model->sample, mInChannels,
                                 DEFAULT_SAMPLE_SIZE, mInSampleFormat, 1);
    if (ret < 0) {
        LOGE("av_samples_fill_arrays error: %s", av_err2str(ret));
        av_frame_free(&srcFrame);
        return ret;
    }
    // src frame 的参数和 buffersrcCtx 的参数必须相同
    srcFrame->sample_rate = mInSampleRate;
    srcFrame->channel_layout = (uint64_t) av_get_default_channel_layout(mInChannels);
    srcFrame->channels = mInChannels;
    srcFrame->format = mInSampleFormat;
    srcFrame->nb_samples = DEFAULT_SAMPLE_SIZE;

    ret = av_buffersrc_add_frame_flags(mABbuffersrcCtx, srcFrame, 0);
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

    ret = av_buffersink_get_frame(mABuffersinkCtx, filtFrame);
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
    mVBuffersrcCtx = nullptr;
    mVBuffersinkCtx = nullptr;
    if (mVFfilterGraph != nullptr) {
        avfilter_graph_free(&mVFfilterGraph);
        mVFfilterGraph = nullptr;
    }
    mABbuffersrcCtx = nullptr;
    mABuffersinkCtx = nullptr;
    if (mAFilterGraph != nullptr) {
        avfilter_graph_free(&mAFilterGraph);
        mAFilterGraph = nullptr;
    }
    if (mParams != nullptr) {
        delete mParams;
        mParams = nullptr;
    }
}

FrameFilter::~FrameFilter() {
    reset();
}
