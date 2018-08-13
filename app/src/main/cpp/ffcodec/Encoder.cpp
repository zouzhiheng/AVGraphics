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
    this->params = params;
    dstFilePath = params->dstFilePath;
    width = params->width;
    height = params->height;
    pixelFormat = params->pixelFormat;
    frameRate = params->frameRate;
    sampleFormat = params->sampleFormat;
    sampleRate = params->sampleRate;
    channels = params->channels;
    videoCodecID =
            params->videoCodecID == AV_CODEC_ID_NONE ? AV_CODEC_ID_H264 : params->videoCodecID;
    audioCodecID =
            params->audioCodecID == AV_CODEC_ID_NONE ? AV_CODEC_ID_AAC : params->audioCodecID;
    maxBitRate = params->maxBitRate;
    haveVideo = params->haveVideo;
    haveAudio = params->haveAudio;
    frameRateFixed = params->frameRateFixed;

    if (width % 2 == 1) {
        if (height >= width) {
            height = (int) (1.0f * (width - 1) / width * height);
            height = height % 2 == 1 ? height - 1 : height;
        }
        width--;
    }

    if (height % 2 == 1) {
        if (width >= height) {
            width = (int) (1.0f * (height - 1) / height * width);
            width = width % 2 == 1 ? width - 1 : width;
        }
        height--;
    }

    formatCtx = nullptr;
    videoCodecCtx = nullptr;
    audioCodecCtx = nullptr;
    videoStream = nullptr;
    audioStream = nullptr;
    picture = nullptr;
    pictureBuf = nullptr;
    tmpPicture = nullptr;
    sample = nullptr;
    sampleBuf = nullptr;
    imgConvertCtx = nullptr;
    sampleConvertCtx = nullptr;

    imageCount = 0;
    sampleCount = 0;
    startPts = 0;
    lastPts = -1;
    sampleBufSize = 0;
    samplePlanes = 0;

    return openOutputFile();
}

int Encoder::openOutputFile() {
    int ret;
    av_register_all();
    av_log_set_callback(androidLog);

    avformat_alloc_output_context2(&formatCtx, nullptr, nullptr, dstFilePath);
    if (!formatCtx) {
        LOGI("Could not create output context");
        return AVERROR_UNKNOWN;
    }

    LOGI("encoder input params: %s", params->toString().c_str());

    if (haveVideo && (ret = openCodecCtx(videoCodecID, AVMEDIA_TYPE_VIDEO)) < 0) {
        LOGE("Open video encoder context failed: %s", av_err2str(ret));
        release();
        return ret;
    }

    if (haveAudio && (ret = openCodecCtx(audioCodecID, AVMEDIA_TYPE_AUDIO)) < 0) {
        LOGE("Open audio encoder context failed: %s", av_err2str(ret));
        release();
        return ret;
    }

    if (haveVideo) {
        picture = av_frame_alloc();
        if (!picture) {
            LOGE("Could not allocate video frame");
            release();
            return FAILED;
        }
        picture->format = pixelFormat;
        picture->width = width;
        picture->height = height;
        picture->pts = 0;

        int size = av_image_get_buffer_size(pixelFormat, width, height, 1);
        if (size < 0) {
            LOGE("image get buffer size error: %s", av_err2str(size));
            release();
            return FAILED;
        }
        pictureBuf = (uint8_t *) av_malloc((size_t) size);
        if (!pictureBuf) {
            LOGE("Could not allocate memory");
            release();
            return FAILED;
        }
        tmpPicture = av_frame_alloc();
        if (!tmpPicture) {
            LOGE("Could not allocate tmp video frame");
            release();
            return FAILED;
        }
    }

    if (haveAudio) {
        sample = av_frame_alloc();
        if (!sample) {
            LOGE("Could not allocate audio frame");
            release();
            return FAILED;
        }
        sample->format = audioCodecCtx->sample_fmt;
        sample->nb_samples = audioCodecCtx->frame_size;
        sample->channel_layout = audioCodecCtx->channel_layout;
        sample->pts = 0;

        samplePlanes = av_sample_fmt_is_planar(audioCodecCtx->sample_fmt) ? audioCodecCtx->channels
                                                                          : 1;
        sampleBufSize = av_samples_get_buffer_size(nullptr, audioCodecCtx->channels,
                                                   audioCodecCtx->frame_size,
                                                   audioCodecCtx->sample_fmt, 1) / samplePlanes;
        sampleBuf = new uint8_t *[samplePlanes];
        for (int i = 0; i < samplePlanes; i++) {
            sampleBuf[i] = (uint8_t *) av_malloc((size_t) sampleBufSize);
            if (sampleBuf[i] == nullptr) {
                LOGE("Could not allocate memory");
                release();
                return FAILED;
            }
        }

        sampleConvertCtx = swr_alloc_set_opts(sampleConvertCtx,
                                              audioCodecCtx->channel_layout,
                                              audioCodecCtx->sample_fmt,
                                              audioCodecCtx->sample_rate,
                                              av_get_default_channel_layout(channels),
                                              sampleFormat, sampleRate, 0, nullptr);
        if (!sampleConvertCtx) {
            LOGE("sampleConvertCtx alloc failed, audio record may be failed!");
        } else if (swr_init(sampleConvertCtx) < 0) {
            LOGE("sampleConvertCtx init failed, audio record may be failed!");
        }
    }

    av_dump_format(formatCtx, 0, dstFilePath, 1);

    if (!(formatCtx->oformat->flags & AVFMT_NOFILE)) {
        if ((ret = avio_open(&formatCtx->pb, dstFilePath, AVIO_FLAG_WRITE)) < 0) {
            LOGE("Could not open output file '%s'", dstFilePath);
            release();
            return ret;
        }
    }

    // init muxer, write output file header
    ret = avformat_write_header(formatCtx, nullptr);
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
    stream = avformat_new_stream(formatCtx, encoder);
    if (!stream) {
        LOGE("Could not allocate avstream.");
        return FAILED;
    }

    if (mediaType == AVMEDIA_TYPE_VIDEO) {
        codecCtx->width = width;
        codecCtx->height = height;
        codecCtx->pix_fmt = pixelFormat;
        // 设置较小的 gop_size，提高截图效率
        codecCtx->gop_size = frameRate;
        if (frameRateFixed) {
            codecCtx->time_base = (AVRational) {1, frameRate};
        } else {
            codecCtx->time_base = (AVRational) {1, 1000};
        }
        if (maxBitRate > 0) {
            codecCtx->rc_max_rate = maxBitRate;
            codecCtx->rc_buffer_size = (int) maxBitRate;
        }

        map<string, string>::iterator it = params->videoMetadata.begin();
        for (; it != params->videoMetadata.end(); it++) {
            av_dict_set(&stream->metadata, (*it).first.c_str(), (*it).second.c_str(), 0);
        }
    } else if (mediaType == AVMEDIA_TYPE_AUDIO) {
        codecCtx->sample_rate = sampleRate;
        codecCtx->channels = channels;
        codecCtx->channel_layout = (uint64_t) av_get_default_channel_layout(channels);
        codecCtx->sample_fmt = encoder->sample_fmts[0];
        codecCtx->time_base = AVRational{1, codecCtx->sample_rate};
    }
    stream->time_base = codecCtx->time_base;

    // some formats want stream headers to be separate
    if (formatCtx->oformat->flags & AVFMT_GLOBALHEADER) {
        codecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    AVDictionary *options = nullptr;
    map<string, string>::iterator it = params->encodeOptions.begin();
    for (; it != params->encodeOptions.end(); it++) {
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
        videoCodecCtx = codecCtx;
        videoStream = stream;
    } else if (mediaType == AVMEDIA_TYPE_AUDIO) {
        audioCodecCtx = codecCtx;
        audioStream = stream;
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
    AVCodecContext *codecCtx = isVideo ? videoCodecCtx : audioCodecCtx;
    AVStream *stream = isVideo ? videoStream : audioStream;
    AVFrame *frame = isVideo ? picture : sample;
    uint8_t *data = isVideo ? model->image : model->sample;
    const char *type = isVideo ? "video" : "audio";

    if ((isVideo && !haveVideo) || (!isVideo && !haveAudio)) {
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
        ret = av_interleaved_write_frame(formatCtx, &packet);
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
    ret = av_image_fill_arrays(picture->data, picture->linesize, model->image,
                               getPixelFormat(model->pixelFormat), model->width, model->height, 1);
    if (ret < 0) {
        LOGE("av_image_fill_arrays error: %s", av_err2str(ret));
        return FAILED;
    }
    if (frameRateFixed) {
        picture->pts = imageCount++;
    } else {
        if (startPts == 0) {
            picture->pts = 0;
            startPts = model->pts;
        } else {
            picture->pts = model->pts - startPts;
        }
        // 少数情况下，当前帧的 pts 和上一帧一样，导致 packet 写入文件失败，进而可能导致花屏
        if (picture->pts == lastPts) {
            picture->pts += 10;
        }
        lastPts = picture->pts;
    }

    return SUCCEED;
}

int Encoder::fillSample(AVModel *model) {
    int ret;
    if (audioCodecCtx->channels != channels || audioCodecCtx->sample_fmt != sampleFormat ||
        audioCodecCtx->sample_rate != sampleRate) {
        ret = swr_convert(sampleConvertCtx, sampleBuf, audioCodecCtx->frame_size,
                          (const uint8_t **) &model->sample, audioCodecCtx->frame_size);
        if (ret <= 0) {
            LOGE("swr_convert error: %s", av_err2str(ret));
            return FAILED;
        }
        avcodec_fill_audio_frame(sample, channels, audioCodecCtx->sample_fmt, sampleBuf[0],
                                 sampleBufSize, 0);
        for (int i = 0; i < samplePlanes; i++) {
            sample->data[i] = sampleBuf[i];
            sample->linesize[i] = sampleBufSize;
        }
    } else {
        ret = av_samples_fill_arrays(sample->data, sample->linesize, model->sample,
                                     audioCodecCtx->channels, sample->nb_samples,
                                     audioCodecCtx->sample_fmt, 1);
    }
    if (ret < 0) {
        LOGE("av_samples_fill_arrays error: %s", av_err2str(ret));
        return FAILED;
    }
    sample->pts = sampleCount;
    sampleCount += sample->nb_samples;

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
    av_write_trailer(formatCtx);
    release();

    return ret;
}

void Encoder::release() {
    if (picture != nullptr) {
        av_frame_free(&picture);
        picture = nullptr;
    }
    if (tmpPicture != nullptr) {
        av_frame_free(&tmpPicture);
        tmpPicture = nullptr;
    }
    if (pictureBuf != nullptr) {
        av_free(pictureBuf);
        pictureBuf = nullptr;
    }
    if (sample != nullptr) {
        av_frame_free(&sample);
        sample = nullptr;
    }
    if (sampleBuf != nullptr) {
        for (int i = 0; i < samplePlanes; i++) {
            if (sampleBuf[i] != nullptr) {
                av_free(sampleBuf[i]);
                sampleBuf[i] = nullptr;
            }
        }
        delete[] sampleBuf;
        sampleBuf = nullptr;
    }
    if (videoCodecCtx != nullptr) {
        avcodec_free_context(&videoCodecCtx);
        videoCodecCtx = nullptr;
    }
    if (audioCodecCtx != nullptr) {
        avcodec_free_context(&audioCodecCtx);
        audioCodecCtx = nullptr;
    }
    if (formatCtx && !(formatCtx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&formatCtx->pb);
        avformat_close_input(&formatCtx);
        formatCtx = nullptr;
    }
    if (imgConvertCtx != nullptr) {
        sws_freeContext(imgConvertCtx);
        imgConvertCtx = nullptr;
    }
    if (sampleConvertCtx != nullptr) {
        swr_free(&sampleConvertCtx);
        sampleConvertCtx = nullptr;
    }
    if (params != nullptr) {
        delete params;
        params = nullptr;
    }
    if (videoStream != nullptr && videoStream->metadata != nullptr) {
        av_dict_free(&videoStream->metadata);
        videoStream->metadata = nullptr;
    }
    videoStream = nullptr;
    audioStream = nullptr;
}

Encoder::~Encoder() {
    release();
    LOGI("Encoder released");
}