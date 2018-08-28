//
// Created by zzh on 2018/8/13 0013.
//

#include "Encoder.h"
#include <android/log.h>
#include <sstream>
#include <libyuv/convert.h>
#include <format.h>
#include <result.h>

using namespace std;
using namespace libyuv;

#define LOG_TAG "ffcodec_encoder"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define FLOGE(fmt, ...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, fmt, __VA_ARGS__)

static void androidLog(void *ptr, int level, const char *fmt, va_list vl) {
    if (level == AV_LOG_ERROR || level == AV_LOG_FATAL) {
        FLOGE(fmt, vl);
    }
}

Encoder::Parameter::Parameter(const char *dstFilePath, int width, int height, int frameRate,
                              int sampleRate, int channels, AVPixelFormat pixelFormat,
                              AVSampleFormat sampleFormat) {
    this->dstFilePath = dstFilePath;
    this->width = width;
    this->height = height;
    this->frameRate = frameRate;
    this->sampleRate = sampleRate;
    this->pixelFormat = pixelFormat;
    this->sampleFormat = sampleFormat;
    this->channels = channels;
    videoCodecID = AV_CODEC_ID_NONE;
    audioCodecID = AV_CODEC_ID_NONE;
    maxBitRate = 0;
    frameRateFixed = true;
    haveVideo = true;
    haveAudio = true;
}

void Encoder::Parameter::setVideoCodec(AVCodecID codecID) {
    videoCodecID = codecID;
}

void Encoder::Parameter::setAudioCodec(AVCodecID codecID) {
    audioCodecID = codecID;
}

void Encoder::Parameter::setMaxBitRate(int64_t maxBitRate) {
    this->maxBitRate = maxBitRate;
}

void Encoder::Parameter::setQuality(int quality) {
    stringstream ss;
    ss << quality;
    string str;
    ss >> str;
    encodeOptions["crf"] = str;
}

void Encoder::Parameter::setFrameRateFixed(bool frameRateFixed) {
    this->frameRateFixed = frameRateFixed;
}

void Encoder::Parameter::setHaveVideo(bool haveVideo) {
    this->haveVideo = haveVideo;
}

void Encoder::Parameter::setHaveAudio(bool haveAudio) {
    this->haveAudio = haveAudio;
}

void Encoder::Parameter::addEncodeOptions(std::string key, std::string value) {
    encodeOptions[key] = value;
}

void Encoder::Parameter::addVideoMetadata(std::string key, std::string value) {
    videoMetadata[key] = value;
}

std::string Encoder::Parameter::toString() {
    char description[1024];
    sprintf(description,
            "{\n[width: %d], [height: %d], [frame rate: %d], [pixel format %s]\n"
                    "[sample rate %d], [sample format: %s], [channels  %d]\n"
                    "[video codec: %s], [audio codec: %s]\n"
                    "[max bit rate: %lld], [frame rate fixed: %d], [have video: %d], [have audio: %d]\n}",
            width, height, frameRate, av_get_pix_fmt_name(pixelFormat), sampleRate,
            av_get_sample_fmt_name(sampleFormat), channels, avcodec_get_name(videoCodecID),
            avcodec_get_name(audioCodecID), maxBitRate, frameRateFixed, haveVideo, haveAudio);
    std::string str = description;
    return str;
}

int Encoder::init(Parameter *params) {
    this->mParams = params;
    mDstFilePath = params->dstFilePath;
    mWidth = params->width;
    mHeight = params->height;
    mPixelFormat = params->pixelFormat;
    mFrameRate = params->frameRate;
    mSampleFormat = params->sampleFormat;
    mSampleRate = params->sampleRate;
    mChannels = params->channels;
    mVideoCodecID =
            params->videoCodecID == AV_CODEC_ID_NONE ? AV_CODEC_ID_H264 : params->videoCodecID;
    mAudioCodecID =
            params->audioCodecID == AV_CODEC_ID_NONE ? AV_CODEC_ID_AAC : params->audioCodecID;
    mMaxBitRate = params->maxBitRate;
    mHaveVideo = params->haveVideo;
    mHaveAudio = params->haveAudio;
    mFrameRateFixed = params->frameRateFixed;

    if (mWidth % 2 == 1) {
        if (mHeight >= mWidth) {
            mHeight = (int) (1.0f * (mWidth - 1) / mWidth * mHeight);
            mHeight = mHeight % 2 == 1 ? mHeight - 1 : mHeight;
        }
        mWidth--;
    }

    if (mHeight % 2 == 1) {
        if (mWidth >= mHeight) {
            mWidth = (int) (1.0f * (mHeight - 1) / mHeight * mWidth);
            mWidth = mWidth % 2 == 1 ? mWidth - 1 : mWidth;
        }
        mHeight--;
    }

    mFormatCtx = nullptr;
    mVideoCodecCtx = nullptr;
    mAudioCodecCtx = nullptr;
    mVideoStream = nullptr;
    mAudioStream = nullptr;
    mPicture = nullptr;
    mPictureBuf = nullptr;
    mTmpPicture = nullptr;
    mSample = nullptr;
    mSampleBuf = nullptr;
    mImgConvertCtx = nullptr;
    mSampleConvertCtx = nullptr;

    mImageCount = 0;
    mSampleCount = 0;
    mStartPts = 0;
    mLastPts = -1;
    mSampleBufSize = 0;
    mSamplePlanes = 0;

    return openOutputFile();
}

int Encoder::openOutputFile() {
    int ret;
    av_register_all();
    av_log_set_callback(androidLog);

    avformat_alloc_output_context2(&mFormatCtx, nullptr, nullptr, mDstFilePath);
    if (!mFormatCtx) {
        LOGI("Could not create output context");
        return AVERROR_UNKNOWN;
    }

    LOGI("encoder input params: %s", mParams->toString().c_str());

    if (mHaveVideo && (ret = openCodecCtx(mVideoCodecID, AVMEDIA_TYPE_VIDEO)) < 0) {
        LOGE("Open video encoder context failed: %s", av_err2str(ret));
        release();
        return ret;
    }

    if (mHaveAudio && (ret = openCodecCtx(mAudioCodecID, AVMEDIA_TYPE_AUDIO)) < 0) {
        LOGE("Open audio encoder context failed: %s", av_err2str(ret));
        release();
        return ret;
    }

    if (mHaveVideo) {
        mPicture = av_frame_alloc();
        if (!mPicture) {
            LOGE("Could not allocate video frame");
            release();
            return FAILED;
        }
        mPicture->format = mPixelFormat;
        mPicture->width = mWidth;
        mPicture->height = mHeight;
        mPicture->pts = 0;

        int size = av_image_get_buffer_size(mPixelFormat, mWidth, mHeight, 1);
        if (size < 0) {
            LOGE("image get buffer size error: %s", av_err2str(size));
            release();
            return FAILED;
        }
        mPictureBuf = (uint8_t *) av_malloc((size_t) size);
        if (!mPictureBuf) {
            LOGE("Could not allocate memory");
            release();
            return FAILED;
        }
        mTmpPicture = av_frame_alloc();
        if (!mTmpPicture) {
            LOGE("Could not allocate tmp video frame");
            release();
            return FAILED;
        }
    }

    if (mHaveAudio) {
        mSample = av_frame_alloc();
        if (!mSample) {
            LOGE("Could not allocate audio frame");
            release();
            return FAILED;
        }
        mSample->format = mAudioCodecCtx->sample_fmt;
        mSample->nb_samples = mAudioCodecCtx->frame_size;
        mSample->channel_layout = mAudioCodecCtx->channel_layout;
        mSample->pts = 0;

        mSamplePlanes = av_sample_fmt_is_planar(mAudioCodecCtx->sample_fmt) ? mAudioCodecCtx->channels
                                                                          : 1;
        mSampleBufSize = av_samples_get_buffer_size(nullptr, mAudioCodecCtx->channels,
                                                   mAudioCodecCtx->frame_size,
                                                   mAudioCodecCtx->sample_fmt, 1) / mSamplePlanes;
        mSampleBuf = new uint8_t *[mSamplePlanes];
        for (int i = 0; i < mSamplePlanes; i++) {
            mSampleBuf[i] = (uint8_t *) av_malloc((size_t) mSampleBufSize);
            if (mSampleBuf[i] == nullptr) {
                LOGE("Could not allocate memory");
                release();
                return FAILED;
            }
        }

        mSampleConvertCtx = swr_alloc_set_opts(mSampleConvertCtx,
                                              mAudioCodecCtx->channel_layout,
                                              mAudioCodecCtx->sample_fmt,
                                              mAudioCodecCtx->sample_rate,
                                              av_get_default_channel_layout(mChannels),
                                              mSampleFormat, mSampleRate, 0, nullptr);
        if (!mSampleConvertCtx) {
            LOGE("sampleConvertCtx alloc failed, audio record may be failed!");
        } else if (swr_init(mSampleConvertCtx) < 0) {
            LOGE("sampleConvertCtx init failed, audio record may be failed!");
        }
    }

    av_dump_format(mFormatCtx, 0, mDstFilePath, 1);

    if (!(mFormatCtx->oformat->flags & AVFMT_NOFILE)) {
        if ((ret = avio_open(&mFormatCtx->pb, mDstFilePath, AVIO_FLAG_WRITE)) < 0) {
            LOGE("Could not open output file '%s'", mDstFilePath);
            release();
            return ret;
        }
    }

    // init muxer, write output file header
    ret = avformat_write_header(mFormatCtx, nullptr);
    if (ret < 0) {
        LOGE("Error occurred when opening output file");
        release();
        return ret;
    }

    LOGI("Encoder init succeed, encoding now...");
    return ret;
}

int Encoder::openCodecCtx(AVCodecID codecID, AVMediaType mediaType) {
    int ret;
    AVCodecContext *codecCtx = nullptr;
    AVStream *stream = nullptr;
    AVCodec *encoder = avcodec_find_encoder(codecID);
    if (!encoder) {
        LOGE("Necessary encoder not found");
        return AVERROR_INVALIDDATA;
    }

    codecCtx = avcodec_alloc_context3(encoder);
    if (!codecCtx) {
        LOGE("Failed to allocate the encoder context");
        return AVERROR(ENOMEM);
    }
    stream = avformat_new_stream(mFormatCtx, encoder);
    if (!stream) {
        LOGE("Could not allocate avstream.");
        return FAILED;
    }

    if (mediaType == AVMEDIA_TYPE_VIDEO) {
        codecCtx->width = mWidth;
        codecCtx->height = mHeight;
        codecCtx->pix_fmt = mPixelFormat;
        // 设置较小的 gop_size，提高截图效率
        codecCtx->gop_size = mFrameRate;
        if (mFrameRateFixed) {
            codecCtx->time_base = (AVRational) {1, mFrameRate};
        } else {
            codecCtx->time_base = (AVRational) {1, 1000};
        }
        if (mMaxBitRate > 0) {
            codecCtx->rc_max_rate = mMaxBitRate;
            codecCtx->rc_buffer_size = (int) mMaxBitRate;
        }

        map<string, string>::iterator it = mParams->videoMetadata.begin();
        for (; it != mParams->videoMetadata.end(); it++) {
            av_dict_set(&stream->metadata, (*it).first.c_str(), (*it).second.c_str(), 0);
        }
    } else if (mediaType == AVMEDIA_TYPE_AUDIO) {
        codecCtx->sample_rate = mSampleRate;
        codecCtx->channels = mChannels;
        codecCtx->channel_layout = (uint64_t) av_get_default_channel_layout(mChannels);
        codecCtx->sample_fmt = encoder->sample_fmts[0];
        codecCtx->time_base = AVRational{1, codecCtx->sample_rate};
    }
    stream->time_base = codecCtx->time_base;

    // some formats want stream headers to be separate
    if (mFormatCtx->oformat->flags & AVFMT_GLOBALHEADER) {
        codecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    AVDictionary *options = nullptr;
    map<string, string>::iterator it = mParams->encodeOptions.begin();
    for (; it != mParams->encodeOptions.end(); it++) {
        av_dict_set(&options, (*it).first.c_str(), (*it).second.c_str(), 0);
    }
    ret = avcodec_open2(codecCtx, encoder, &options);
    if (ret < 0) {
        LOGE("Could not open %s codec: %s", mediaType == AVMEDIA_TYPE_VIDEO ? "video" : "audio",
             av_err2str(ret));
        av_dict_free(&options);
        return ret;
    }
    av_dict_free(&options);

    ret = avcodec_parameters_from_context(stream->codecpar, codecCtx);
    if (ret < 0) {
        LOGE("Failed to copy encoder parameters to video stream");
        return ret;
    }

    if (mediaType == AVMEDIA_TYPE_VIDEO) {
        mVideoCodecCtx = codecCtx;
        mVideoStream = stream;
    } else if (mediaType == AVMEDIA_TYPE_AUDIO) {
        mAudioCodecCtx = codecCtx;
        mAudioStream = stream;
    }

    return ret;
}

int Encoder::encode(AVModel *model) {
    return encode(model, nullptr);
}

int Encoder::encode(AVModel *model, int *gotFrame) {
    int ret = 0, gotFrameLocal;
    if (!gotFrame) {
        gotFrame = &gotFrameLocal;
    }
    *gotFrame = 0;

    bool isVideo = model->flag == MODEL_FLAG_VIDEO;
    AVCodecContext *codecCtx = isVideo ? mVideoCodecCtx : mAudioCodecCtx;
    AVStream *stream = isVideo ? mVideoStream : mAudioStream;
    AVFrame *frame = isVideo ? mPicture : mSample;
    uint8_t *data = isVideo ? model->image : model->sample;
    const char *type = isVideo ? "video" : "audio";

    if ((isVideo && !mHaveVideo) || (!isVideo && !mHaveAudio)) {
        LOGE("%s model cannot write, no corresponding encoder", type);
        return 0;
    }

    if (data != nullptr) {
        ret = isVideo ? fillPicture(model) : fillSample(model);
        if (ret < 0) {
            return FAILED;
        }
    }

    AVPacket packet;
    packet.data = nullptr;
    packet.size = 0;
    av_init_packet(&packet);

    ret = avcodec_send_frame(codecCtx, data == nullptr ? nullptr : frame);
    if (ret < 0) {
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return 0;
        }
        LOGE("Error sending a %s frame: %s", type, av_err2str(ret));
        return ret;
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(codecCtx, &packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            LOGE("Error during encoding %s", type);
            return ret;
        }

        av_packet_rescale_ts(&packet, codecCtx->time_base, stream->time_base);
        packet.stream_index = stream->index;
        ret = av_interleaved_write_frame(mFormatCtx, &packet);
        if (ret < 0) {
            LOGE("Error writing %s frame: %s", type, av_err2str(ret));
            return ret;
        }
        *gotFrame = 1;
    }

    return SUCCEED;
}

int Encoder::fillPicture(AVModel *model) {
    int ret;
    ret = av_image_fill_arrays(mPicture->data, mPicture->linesize, model->image,
                               getPixelFormat(model->pixelFormat), model->width, model->height, 1);
    if (ret < 0) {
        LOGE("av_image_fill_arrays error: %s, [%d, %d, %s], [%d, %d], [%d, %d, %s]",
             av_err2str(ret), mPicture->width, mPicture->height,
             av_get_pix_fmt_name((AVPixelFormat) mPicture->format), mWidth, mHeight,
             model->width, model->height, av_get_pix_fmt_name(getPixelFormat(model->pixelFormat)));
        return FAILED;
    }
    if (mFrameRateFixed) {
        mPicture->pts = mImageCount++;
    } else {
        if (mStartPts == 0) {
            mPicture->pts = 0;
            mStartPts = model->pts;
        } else {
            mPicture->pts = model->pts - mStartPts;
        }
        // 少数情况下，当前帧的 pts 和上一帧一样，导致 packet 写入文件失败，进而可能导致花屏
        if (mPicture->pts == mLastPts) {
            mPicture->pts += 10;
        }
        mLastPts = mPicture->pts;
    }

    return SUCCEED;
}

int Encoder::fillSample(AVModel *model) {
    int ret;
    if (mAudioCodecCtx->channels != mChannels || mAudioCodecCtx->sample_fmt != mSampleFormat ||
        mAudioCodecCtx->sample_rate != mSampleRate) {
        ret = swr_convert(mSampleConvertCtx, mSampleBuf, mAudioCodecCtx->frame_size,
                          (const uint8_t **) &model->sample, mAudioCodecCtx->frame_size);
        if (ret <= 0) {
            LOGE("swr_convert error: %s", av_err2str(ret));
            return FAILED;
        }
        avcodec_fill_audio_frame(mSample, mChannels, mAudioCodecCtx->sample_fmt, mSampleBuf[0],
                                 mSampleBufSize, 0);
        for (int i = 0; i < mSamplePlanes; i++) {
            mSample->data[i] = mSampleBuf[i];
            mSample->linesize[i] = mSampleBufSize;
        }
    } else {
        ret = av_samples_fill_arrays(mSample->data, mSample->linesize, model->sample,
                                     mAudioCodecCtx->channels, mSample->nb_samples,
                                     mAudioCodecCtx->sample_fmt, 1);
    }
    if (ret < 0) {
        LOGE("av_samples_fill_arrays error: %s", av_err2str(ret));
        return FAILED;
    }
    mSample->pts = mSampleCount;
    mSampleCount += mSample->nb_samples;

    return SUCCEED;
}

int Encoder::stop() {
    int ret = 0, gotFrame;
    LOGI("Flushing video encoder");
    AVModel *model = new AVModel();
    model->flag = MODEL_FLAG_VIDEO;
    while (1) {
        ret = encode(model, &gotFrame);
        if (ret < 0 || !gotFrame) {
            break;
        }
    }

    LOGI("Flushing audio encoder");
    model->flag = MODEL_FLAG_AUDIO;
    while (1) {
        ret = encode(model, &gotFrame);
        if (ret < 0 || !gotFrame) {
            break;
        }
    }
    delete model;
    av_write_trailer(mFormatCtx);
    release();

    return ret;
}

void Encoder::release() {
    if (mPicture != nullptr) {
        av_frame_free(&mPicture);
        mPicture = nullptr;
    }
    if (mTmpPicture != nullptr) {
        av_frame_free(&mTmpPicture);
        mTmpPicture = nullptr;
    }
    if (mPictureBuf != nullptr) {
        av_free(mPictureBuf);
        mPictureBuf = nullptr;
    }
    if (mSample != nullptr) {
        av_frame_free(&mSample);
        mSample = nullptr;
    }
    if (mSampleBuf != nullptr) {
        for (int i = 0; i < mSamplePlanes; i++) {
            if (mSampleBuf[i] != nullptr) {
                av_free(mSampleBuf[i]);
                mSampleBuf[i] = nullptr;
            }
        }
        delete[] mSampleBuf;
        mSampleBuf = nullptr;
    }
    if (mVideoCodecCtx != nullptr) {
        avcodec_free_context(&mVideoCodecCtx);
        mVideoCodecCtx = nullptr;
    }
    if (mAudioCodecCtx != nullptr) {
        avcodec_free_context(&mAudioCodecCtx);
        mAudioCodecCtx = nullptr;
    }
    if (mFormatCtx && !(mFormatCtx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&mFormatCtx->pb);
        avformat_close_input(&mFormatCtx);
        mFormatCtx = nullptr;
    }
    if (mImgConvertCtx != nullptr) {
        sws_freeContext(mImgConvertCtx);
        mImgConvertCtx = nullptr;
    }
    if (mSampleConvertCtx != nullptr) {
        swr_free(&mSampleConvertCtx);
        mSampleConvertCtx = nullptr;
    }
    if (mParams != nullptr) {
        delete mParams;
        mParams = nullptr;
    }
    if (mVideoStream != nullptr && mVideoStream->metadata != nullptr) {
        av_dict_free(&mVideoStream->metadata);
        mVideoStream->metadata = nullptr;
    }
    mVideoStream = nullptr;
    mAudioStream = nullptr;
}

Encoder::~Encoder() {
    release();
    LOGI("Encoder released");
}