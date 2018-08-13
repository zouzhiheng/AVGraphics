//
// Created by Administrator on 2018/3/2 0002.
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
    videoStreamIdx = -1;
    audioStreamIdx = -1;
    srcFilePath = nullptr;

    formatCtx = nullptr;
    videoCodecCtx = nullptr;
    audioCodecCtx = nullptr;
    videoStream = nullptr;
    audioStream = nullptr;
    frame = nullptr;
    for (int i = 0; i < 4; i++) {
        videoBuffer[i] = nullptr;
    }

    yuvFile = nullptr;
    pcmFile = nullptr;
}

int Decoder::decode(const char *srcFilePath, const char *dstVideoPath, const char *dstAudioPath) {
    yuvFile = fopen(dstVideoPath, "wb");
    pcmFile = fopen(dstAudioPath, "wb");
    return doDecode(srcFilePath);
}

int Decoder::doDecode(const char *srcFilePath) {
    this->srcFilePath = srcFilePath;

    int ret, gotFrame;
    if (openInputFile() < 0) {
        LOGE("open input file failed!");
        return FAILED;
    }

    LOGI("Decoder init succeed, decoding now...");
    while (av_read_frame(formatCtx, &packet) >= 0) {
        AVPacket origin = packet;
        do {
            if ((ret = decodePacket(packet, &gotFrame)) < 0) {
                break;
            }
            packet.data += ret;
            packet.size -= ret;
        } while (packet.size > 0);
        av_packet_unref(&origin);
    }

    LOGI("Flushing decoder");
    // flush cached frames
    packet.data = nullptr;
    packet.size = 0;
    do {
        decodePacket(packet, &gotFrame);
    } while (gotFrame);

    release();

    return SUCCEED;
}

int Decoder::openInputFile() {

    int ret = 0;

    av_register_all();
    av_log_set_callback(androidLog);

    if (avformat_open_input(&formatCtx, srcFilePath, nullptr, nullptr) < 0) {
        LOGE("Could not open source file: %s", srcFilePath);
        return FAILED;
    }

    if (avformat_find_stream_info(formatCtx, nullptr) < 0) {
        LOGE("Could not find stream info");
        return FAILED;
    }

    if (openCodecCtx(&videoStreamIdx, &videoCodecCtx, formatCtx, AVMEDIA_TYPE_VIDEO) == SUCCEED) {
        videoStream = formatCtx->streams[videoStreamIdx];
        width = videoCodecCtx->width;
        height = videoCodecCtx->height;
        pixelFormat = videoCodecCtx->pix_fmt;
        frameRate = (int) av_q2d(av_guess_frame_rate(formatCtx, videoStream, nullptr));
        videoCodecCtx->time_base = av_inv_q(av_d2q(frameRate, 100000));
        ret = av_image_alloc(videoBuffer, videoLinesize, width, height, pixelFormat, 1);
        if (ret < 0) {
            LOGE("Could not allocate raw video buffer");
            release();
            return FAILED;
        }
        videoBufferSize = ret;
    }

    if (openCodecCtx(&audioStreamIdx, &audioCodecCtx, formatCtx, AVMEDIA_TYPE_AUDIO) == SUCCEED) {
        audioStream = formatCtx->streams[audioStreamIdx];
        sampleFormat = audioCodecCtx->sample_fmt;
        sampleRate = audioCodecCtx->sample_rate;
        channels = audioCodecCtx->channels;
    }

    av_dump_format(formatCtx, 0, srcFilePath, 0);

    if (!audioStream && !videoStream) {
        LOGE("Could not find audio or video stream in the input, aborting");
        release();
        return FAILED;
    }

    frame = av_frame_alloc();
    if (!frame) {
        LOGE("Could not allocate frame");
        release();
        return FAILED;
    }

    av_init_packet(&packet);
    packet.data = nullptr;
    packet.size = 0;

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
             av_get_media_type_string(mediaType), srcFilePath, av_err2str(ret));
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

int Decoder::decodePacket(AVPacket packet, int *gotFrame) {

    int ret = 0;
    int decoded = packet.size;

    *gotFrame = 0;
    if (packet.stream_index == videoStreamIdx) {
        ret = avcodec_decode_video2(videoCodecCtx, frame, gotFrame, &packet);
        if (ret < 0) {
            LOGE("Error decoding video frame: %s", av_err2str(ret));
            return FAILED;
        }

        if (*gotFrame) {
            av_image_copy(videoBuffer, videoLinesize, (const uint8_t **) frame->data,
                          frame->linesize, pixelFormat, width, height);

            frame->pts = frame->best_effort_timestamp;

            fwrite(videoBuffer[0], 1, (size_t) videoBufferSize, yuvFile);
        }
    } else if (packet.stream_index == audioStreamIdx) {
        ret = avcodec_decode_audio4(audioCodecCtx, frame, gotFrame, &packet);
        if (ret < 0) {
            LOGE("Error decoding audio frame: %s", av_err2str(ret));
            return FAILED;
        }
        /* Some audio decoders decode only part of the packet, and have to be
         * called again with the remainder of the packet data.
         * Also, some decoders might over-read the packet. */
        decoded = FFMIN(ret, packet.size);

        if (*gotFrame) {
            int unpaddingLinesize =
                    frame->nb_samples * av_get_bytes_per_sample(AVSampleFormat(frame->format));
            fwrite(frame->extended_data[0], 1, (size_t) unpaddingLinesize, pcmFile);
        }
    }

    if (*gotFrame) {
        av_frame_unref(frame);
    }

    return decoded;
}

int Decoder::release() {
    if (videoCodecCtx != nullptr) {
        avcodec_free_context(&videoCodecCtx);
    }
    if (audioCodecCtx != nullptr) {
        avcodec_free_context(&audioCodecCtx);
    }
    if (formatCtx != nullptr) {
        avformat_close_input(&formatCtx);
    }
    if (frame != nullptr) {
        av_frame_free(&frame);
    }
    if (videoBuffer[0] != nullptr) {
        av_free(videoBuffer[0]);
    }
    if (pcmFile != nullptr) {
        fclose(pcmFile);
    }
    if (yuvFile != nullptr) {
        fclose(yuvFile);
    }
    reset();
    LOGI("Decoder released");

    return SUCCEED;
}


