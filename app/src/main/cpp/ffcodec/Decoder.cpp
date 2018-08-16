//
// Created by zzh on 2018/8/13 0013.
//

#include "Decoder.h"
#include "result.h"
#include <algorithm>
#include <android/log.h>

#define LOG_TAG "ffcodec_decoder"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define FLOGE(fmt, ...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, fmt, __VA_ARGS__)

static void androidLog(void *ptr, int level, const char *fmt, va_list vl) {
    if (level == AV_LOG_ERROR || level == AV_LOG_FATAL) {
        FLOGE(fmt, vl);
    }
}

Decoder::Decoder() {
    reset();
}

void Decoder::reset() {
    mVideoStreamIdx = -1;
    mAudioStreamIdx = -1;
    mSrcFilePath = nullptr;

    mFormatCtx = nullptr;
    mVideoCodecCtx = nullptr;
    mAudioCodecCtx = nullptr;
    mFrame = nullptr;
    for (int i = 0; i < 4; i++) {
        mVideoBuffers[i] = nullptr;
    }

    mYuvFile = nullptr;
    mPcmFile = nullptr;
}

AVInfo *Decoder::getAVInfo(const char *srcFilePath) {
    AVInfo *info = new AVInfo();
    mSrcFilePath = srcFilePath;
    if (openInputFile() < 0) {
        LOGE("open input file failed!");
        return info;
    }

    if (mVideoCodecCtx != nullptr) {
        info->width = mWidth;
        info->height = mHeight;
        info->pixelFormat = mPixelFormat;
        info->frameRate = mFrameRate;
        info->videoCodecID = mVideoCodecCtx->codec_id;
        info->haveVideo = true;
    }

    if (mAudioCodecCtx != nullptr) {
        info->sampleRate = mAudioCodecCtx->sample_rate;
        info->audioCodecID = mAudioCodecCtx->codec_id;
        info->channels = mAudioCodecCtx->channels;
        info->sampleFormat = mAudioCodecCtx->sample_fmt;
        info->haveAudio = true;
    }
    info->duration = mFormatCtx->duration;
    info->bitRate = mFormatCtx->bit_rate;
    release();

    return info;
}

int Decoder::decode(const char *srcFilePath, const char *dstVideoPath, const char *dstAudioPath) {
    mYuvFile = fopen(dstVideoPath, "wb");
    mPcmFile = fopen(dstAudioPath, "wb");
    return doDecode(srcFilePath);
}

int Decoder::doDecode(const char *srcFilePath) {
    mSrcFilePath = srcFilePath;

    if (openInputFile() < 0) {
        LOGE("open input file failed!");
        return FAILED;
    }

    LOGI("Decoder init succeed, decoding now...");
    while (av_read_frame(mFormatCtx, &mPacket) >= 0) {
        decodePacket(mPacket, nullptr);
        av_packet_unref(&mPacket);
    }

    LOGI("Flushing decoder");
    mPacket.data = nullptr;
    mPacket.size = 0;
    int gotFrame;
    do {
        decodePacket(mPacket, &gotFrame);
    } while (gotFrame);

    release();

    return SUCCEED;
}

int Decoder::openInputFile() {

    int ret = 0;

    av_register_all();
    av_log_set_callback(androidLog);

    if (avformat_open_input(&mFormatCtx, mSrcFilePath, nullptr, nullptr) < 0) {
        LOGE("Could not open source file: %s", mSrcFilePath);
        return FAILED;
    }

    if (avformat_find_stream_info(mFormatCtx, nullptr) < 0) {
        LOGE("Could not find stream info");
        return FAILED;
    }

    AVStream *videoStream = nullptr;
    AVStream *audioStream = nullptr;

    if (openCodecCtx(&mVideoStreamIdx, &mVideoCodecCtx, mFormatCtx, AVMEDIA_TYPE_VIDEO) ==
        SUCCEED) {
        videoStream = mFormatCtx->streams[mVideoStreamIdx];
        mWidth = mVideoCodecCtx->width;
        mHeight = mVideoCodecCtx->height;
        mPixelFormat = mVideoCodecCtx->pix_fmt;
        mFrameRate = (int) av_q2d(av_guess_frame_rate(mFormatCtx, videoStream, nullptr));
        mVideoCodecCtx->time_base = av_inv_q(av_d2q(mFrameRate, 100000));
        ret = av_image_alloc(mVideoBuffers, mVideoLinesize, mWidth, mHeight, mPixelFormat, 1);
        if (ret < 0) {
            LOGE("Could not allocate raw video buffer");
            release();
            return FAILED;
        }
        mVideoBufferSize = ret;
    }

    if (openCodecCtx(&mAudioStreamIdx, &mAudioCodecCtx, mFormatCtx, AVMEDIA_TYPE_AUDIO) ==
        SUCCEED) {
        audioStream = mFormatCtx->streams[mAudioStreamIdx];
        mSampleRate = mAudioCodecCtx->sample_rate;
    }

    av_dump_format(mFormatCtx, 0, mSrcFilePath, 0);

    if (!audioStream && !videoStream) {
        LOGE("Could not find audio or video stream in the input, aborting");
        release();
        return FAILED;
    }

    mFrame = av_frame_alloc();
    if (!mFrame) {
        LOGE("Could not allocate mFrame");
        release();
        return FAILED;
    }

    av_init_packet(&mPacket);
    mPacket.data = nullptr;
    mPacket.size = 0;

    return SUCCEED;
}

int Decoder::openCodecCtx(int *streamIdx, AVCodecContext **codecCtx, AVFormatContext *formatContext,
                          AVMediaType mediaType) {
    int ret, index;
    AVStream *stream;
    AVCodec *codec = nullptr;

    ret = av_find_best_stream(formatContext, mediaType, -1, -1, nullptr, 0);
    if (ret < 0) {
        LOGE("Could not find %s stream in input file '%s', result: %s",
             av_get_media_type_string(mediaType), mSrcFilePath, av_err2str(ret));
        return FAILED;
    } else {
        index = ret;
        stream = formatContext->streams[index];

        codec = avcodec_find_decoder(stream->codecpar->codec_id);
        if (!codec) {
            LOGE("Failed to find %s codec", av_get_media_type_string(mediaType));
            return FAILED;
        }

        *codecCtx = avcodec_alloc_context3(codec);
        if (!*codecCtx) {
            LOGE("Failed to alloc the %s codec context", av_get_media_type_string(mediaType));
            return FAILED;
        }

        if ((ret = avcodec_parameters_to_context(*codecCtx, stream->codecpar)) < 0) {
            LOGE("Failed to copy %s codec parameters to decoder context, result: %d",
                 av_get_media_type_string(mediaType), ret);
            return FAILED;
        }

        if ((ret = avcodec_open2(*codecCtx, codec, nullptr)) < 0) {
            LOGE("Failed to open %s codec, result: %d", av_get_media_type_string(mediaType), ret);
            return FAILED;
        }
        *streamIdx = index;
    }

    return SUCCEED;
}

void Decoder::decodePacket(AVPacket packet, int *gotFrame) {
    int ret = 0;
    int localGotFrame;
    if (!gotFrame)
        gotFrame = &localGotFrame;

    *gotFrame = 0;
    if (packet.stream_index == mVideoStreamIdx) {
        ret = avcodec_send_packet(mVideoCodecCtx, &packet);
        if (ret < 0) {
            LOGE("Error decoding video mFrame: %s", av_err2str(ret));
            return;
        }

        while (ret >= 0) {
            ret = avcodec_receive_frame(mVideoCodecCtx, mFrame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            } else if (ret < 0) {
                LOGE("Error during decoding: %s", av_err2str(ret));
                break;
            }

            av_image_copy(mVideoBuffers, mVideoLinesize, (const uint8_t **) mFrame->data,
                          mFrame->linesize, mPixelFormat, mWidth, mHeight);
            fwrite(mVideoBuffers[0], 1, (size_t) mVideoBufferSize, mYuvFile);

            *gotFrame = 1;
        }
    } else if (packet.stream_index == mAudioStreamIdx) {
        ret = avcodec_send_packet(mAudioCodecCtx, &packet);
        if (ret < 0) {
            LOGE("Error decoding video mFrame: %s", av_err2str(ret));
            return;
        }

        while (ret >= 0) {
            ret = avcodec_receive_frame(mAudioCodecCtx, mFrame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            } else if (ret < 0) {
                LOGE("Error during decoding: %s", av_err2str(ret));
                break;
            }

            int dataSize =
                    mFrame->nb_samples * av_get_bytes_per_sample(AVSampleFormat(mFrame->format));
            fwrite(mFrame->data[0], 1, (size_t) dataSize, mPcmFile);

            *gotFrame = 1;
        }
    }

    if (*gotFrame) {
        av_frame_unref(mFrame);
    }
}

int Decoder::release() {
    if (mVideoCodecCtx != nullptr) {
        avcodec_free_context(&mVideoCodecCtx);
    }
    if (mAudioCodecCtx != nullptr) {
        avcodec_free_context(&mAudioCodecCtx);
    }
    if (mFormatCtx != nullptr) {
        avformat_close_input(&mFormatCtx);
    }
    if (mFrame != nullptr) {
        av_frame_free(&mFrame);
    }
    if (mVideoBuffers[0] != nullptr) {
        av_free(mVideoBuffers[0]);
    }
    if (mPcmFile != nullptr) {
        fclose(mPcmFile);
    }
    if (mYuvFile != nullptr) {
        fclose(mYuvFile);
    }
    reset();
    LOGI("Decoder released");

    return SUCCEED;
}


