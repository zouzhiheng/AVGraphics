//
// Created by zzh on 2018/3/5 0005.
//

#include "Transcoder.h"
#include "result.h"
#include "format.h"
#include <sstream>
#include <android/log.h>
#include <vector>

using namespace std;

#define LOG_TAG "ffcodec_transcoder"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define FLOGE(fmt, ...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, fmt, __VA_ARGS__)

#define MAX_DEVIATION (2 * AV_TIME_BASE)
#define DEVIATION_TOO_LARGE -1234567
#define FIRST_VIDEO_PTS -1234567
#define FIRST_AUDIO_PTS -1234567

int getFrameCount(const char *filePath);

void setEncoderSize(AVCodecContext *codecCtx, int width, int height);

static void androidLog(void *ptr, int level, const char *fmt, va_list vl) {
    if (level == AV_LOG_ERROR || level == AV_LOG_FATAL) {
        FLOGE(fmt, vl);
    }
}

Transcoder::Options::Options() {
    start = 0;
    duration = 0;
    videoFilter = "null";
    audioFilter = "anull";
    maxBitRate = 0;
    reencode = false;
}

void Transcoder::Options::setCutTime(uint64_t start, uint64_t duration) {
    this->start = start;
    this->duration = duration;
}

void Transcoder::Options::setVideoFilter(const char *videoFilter) {
    this->videoFilter = videoFilter == nullptr ? "null" : videoFilter;
}

void Transcoder::Options::setAudioFilter(const char *audioFilter) {
    this->audioFilter = audioFilter == nullptr ? "anull" : audioFilter;
}

void Transcoder::Options::setMaxBitRate(int64_t maxBitRate) {
    this->maxBitRate = maxBitRate;
}

void Transcoder::Options::setQuality(int quality) {
    stringstream ss;
    ss << quality;
    string str;
    ss >> str;
    encodeOptions["crf"] = str;
}

void Transcoder::Options::setReencode(bool reencode) {
    this->reencode = reencode;
}

void Transcoder::Options::addEncodeOptions(std::string key, std::string value) {
    encodeOptions[key] = value;
}

Transcoder::Transcoder() {
    reset();
}

void Transcoder::reset() {
    ifmtCtx = nullptr;
    ofmtCtx = nullptr;
    filterCtx = nullptr;
    streamCtx = nullptr;

    tmpFrame = nullptr;
    tmpFrameBuf = nullptr;
    imgConvertCtx = nullptr;
    sampleConvertCtx = nullptr;
    audioFifo = nullptr;

    options = nullptr;
    totalFrame = 0;
    viStreamIndex = -1;
    aoStreamIndex = -1;
    videoPts = FIRST_VIDEO_PTS;
    audioPts = FIRST_AUDIO_PTS;
    audioPtsDelay = FIRST_AUDIO_PTS;
}

int Transcoder::transcode(const char *srcFilePath, const char *dstFilePath, int64_t start,
                          int64_t duration) {
    AVOutputFormat *ofmt = nullptr;
    AVFormatContext *ifmtCtx = nullptr, *ofmtCtx = nullptr;
    AVPacket packet;
    int ret, i;
    int streamIndex = 0;
    int *streamMap = nullptr;
    int streamMappSize = 0;
    bool startTranscode = false;
    bool endTranscode = false;
    int64_t firstPts = -1;
    int64_t firstVideoPts = -1;
    int64_t firstAudioPts = -1;
    int64_t firstSubtitlePts = -1;
    int64_t videoDelay = 0;
    int64_t audioDelay = 0;
    int64_t subtitleDelay = 0;
    double firstTime = 0;
    AVStream *firstStream = nullptr;
    AVStream *videoStream = nullptr;
    int64_t oldStart = start;
    // 由于 B 帧的存在，转码可能会过早结束，因此添加 0.3s
    duration = duration == 0 ? 0 : duration + 300 * 1000;

    av_register_all();
    av_log_set_callback(androidLog);

    if ((ret = avformat_open_input(&ifmtCtx, srcFilePath, 0, 0)) < 0) {
        LOGE("Could not open input file '%s'", srcFilePath);
        goto end;
    }

    if ((ret = avformat_find_stream_info(ifmtCtx, 0)) < 0) {
        LOGE("Failed to retrieve input stream information");
        goto end;
    }

    av_dump_format(ifmtCtx, 0, srcFilePath, 0);

    avformat_alloc_output_context2(&ofmtCtx, nullptr, nullptr, dstFilePath);
    if (!ofmtCtx) {
        LOGE("Could not create output context");
        ret = AVERROR_UNKNOWN;
        goto end;
    }

    streamMappSize = ifmtCtx->nb_streams;
    streamMap = (int *) av_mallocz_array((size_t) streamMappSize, sizeof(*streamMap));
    if (!streamMap) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    ofmt = ofmtCtx->oformat;

    for (i = 0; i < ifmtCtx->nb_streams; i++) {
        AVStream *outStream;
        AVStream *inStream = ifmtCtx->streams[i];
        AVCodecParameters *inCodecpar = inStream->codecpar;

        if (inCodecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
            inCodecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
            inCodecpar->codec_type != AVMEDIA_TYPE_SUBTITLE) {
            streamMap[i] = -1;
            continue;
        }

        if (inCodecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = inStream;
        }

        streamMap[i] = streamIndex++;

        outStream = avformat_new_stream(ofmtCtx, nullptr);
        if (!outStream) {
            LOGE("Failed allocating output stream");
            ret = AVERROR_UNKNOWN;
            goto end;
        }

        ret = avcodec_parameters_copy(outStream->codecpar, inCodecpar);
        if (ret < 0) {
            LOGE("Failed to copy codec parameters");
            goto end;
        }
        outStream->codecpar->codec_tag = 0;
    }
    av_dump_format(ofmtCtx, 0, dstFilePath, 1);

    if (!(ofmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ofmtCtx->pb, dstFilePath, AVIO_FLAG_WRITE);
        if (ret < 0) {
            LOGE("Could not open output file '%s'", dstFilePath);
            goto end;
        }
    }

    ret = avformat_write_header(ofmtCtx, nullptr);
    if (ret < 0) {
        LOGE("Error occurred when opening output file");
        goto end;
    }

    if (start != 0) {
        int64_t nearlyPts = getNearlyIFramePts(srcFilePath, start) + 10 * 1000;
        int64_t deviation = nearlyPts > start ? nearlyPts - start : start - nearlyPts;
        if (deviation > MAX_DEVIATION) {
            LOGW("can not find a nearly pts with deviation small enough, switch to reencode way");
            ret = DEVIATION_TOO_LARGE;
            goto end;
        }
        if (videoStream != nullptr && seekFrame(ifmtCtx, videoStream, nearlyPts) >= 0) {
            start = -1;
        }
    }

    LOGI("Transcoder init succeed, transcoding now(without reencoding)...");
    while (1) {
        AVStream *inStream, *outStream;

        ret = av_read_frame(ifmtCtx, &packet);
        if (ret < 0)
            break;

        inStream = ifmtCtx->streams[packet.stream_index];
        if (packet.stream_index >= streamMappSize ||
            streamMap[packet.stream_index] < 0) {
            av_packet_unref(&packet);
            continue;
        }

        packet.stream_index = streamMap[packet.stream_index];
        outStream = ofmtCtx->streams[packet.stream_index];

        packet.pts = av_rescale_q_rnd(packet.pts, inStream->time_base, outStream->time_base,
                                      (AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        packet.dts = av_rescale_q_rnd(packet.dts, inStream->time_base, outStream->time_base,
                                      (AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        packet.duration = av_rescale_q(packet.duration, inStream->time_base, outStream->time_base);
        packet.pos = -1;

        // 切割视频
        if (duration != 0) {
            double showTime = packet.pts * av_q2d(outStream->time_base) * AV_TIME_BASE;
            if (start == -1) {
                start = (int64_t) showTime;
            }
            // 因为 B 帧 pts 非递增，可能在结束转码之后，仍然有一些帧会被写到文件
            // 而导致末尾花屏的现象，startTranscode 同理
            if ((showTime < start && !startTranscode) || endTranscode) {
                continue;
            } else if (showTime > start + duration) {
                endTranscode = true;
                continue;
            }

            if (start != 0) {
                AVMediaType mediaType = inStream->codecpar->codec_type;
                int64_t *pts = &firstVideoPts;
                int64_t *delay = &videoDelay;
                if (mediaType == AVMEDIA_TYPE_AUDIO) {
                    pts = &firstAudioPts;
                    delay = &audioDelay;
                } else if (mediaType == AVMEDIA_TYPE_SUBTITLE) {
                    pts = &firstSubtitlePts;
                    delay = &subtitleDelay;
                }

                // 第一帧需要是关键帧，如果视频没有写，则音频也不写
                // 考虑到音视频同步问题，即使音频帧为非关键帧，如果视频已经写入，音频也要跟着写
                if (firstVideoPts == -1 &&
                    (mediaType != AVMEDIA_TYPE_VIDEO || !(packet.flags & AV_PKT_FLAG_KEY))) {
                    if (mediaType == AVMEDIA_TYPE_VIDEO) {
                        duration += packet.duration * av_q2d(outStream->time_base) * AV_TIME_BASE;
                    }
                    continue;
                }

                if (*pts == -1 && packet.pts != AV_NOPTS_VALUE) {
                    *pts = packet.pts;
                    if (firstPts == -1) {
                        firstPts = *pts;
                        firstStream = outStream;
                        packet.pts = 0;
                        packet.dts = packet.dts == AV_NOPTS_VALUE ? packet.dts : 0;
                    } else {
                        firstTime = firstPts * av_q2d(firstStream->time_base) * AV_TIME_BASE;
                        double currentTime = (*pts) * av_q2d(outStream->time_base) * AV_TIME_BASE;
                        *delay = av_rescale_q((int64_t) (currentTime - firstTime), AV_TIME_BASE_Q,
                                              outStream->time_base);
                        packet.pts = *delay;
                        packet.dts = packet.dts == AV_NOPTS_VALUE ? packet.dts : *delay;
                    }
                } else {
                    if (packet.pts != AV_NOPTS_VALUE) {
                        packet.pts = packet.pts - *pts + *delay;
                    }
                    if (packet.dts != AV_NOPTS_VALUE) {
                        packet.dts = packet.dts - *pts + *delay;
                    }
                }
            }
        }
        startTranscode = true;

        ret = av_interleaved_write_frame(ofmtCtx, &packet);
        if (ret < 0) {
            LOGE("Error muxing packet");
            break;
        }
        av_packet_unref(&packet);
    }

    if (!startTranscode || abs((int) (start - oldStart)) > MAX_DEVIATION) {
        LOGW("Transcode without reencoding failed, change to reencode way");
        ret = DEVIATION_TOO_LARGE;
        goto end;
    }

    av_write_trailer(ofmtCtx);
    LOGI("Transcode succeed!");

    end:
    avformat_close_input(&ifmtCtx);
    if (ofmtCtx && !(ofmt->flags & AVFMT_NOFILE))
        avio_closep(&ofmtCtx->pb);
    avformat_free_context(ofmtCtx);
    av_freep(&streamMap);

    if (ret < 0 && ret != AVERROR_EOF) {
        if (ret != DEVIATION_TOO_LARGE) {
            LOGE("Error occurred: %s", av_err2str(ret));
        }
        return ret;
    }

    return SUCCEED;
}

int64_t Transcoder::getNearlyIFramePts(const char *filePath, int64_t targetPts) {
    return getNearlyIFramePts(filePath, targetPts, false);
}

int64_t Transcoder::getNearlyIFramePts(const char *filePath, int64_t targetPts, bool backward) {
    AVFormatContext *formatCtx = nullptr;
    AVStream *videoStream = nullptr;
    AVPacket packet = {.size = 0, .data = nullptr};
    int64_t result = 0;

    av_register_all();

    if (avformat_open_input(&formatCtx, filePath, 0, 0) < 0) {
        LOGE("Could not open input file '%s'", filePath);
        return result;
    }

    if (avformat_find_stream_info(formatCtx, 0) < 0) {
        LOGE("Could not open input file '%s'", filePath);
        return result;
    }

    av_dump_format(formatCtx, 0, filePath, 0);

    for (int i = 0; i < formatCtx->nb_streams; ++i) {
        if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = formatCtx->streams[i];
            break;
        }
    }

    if (videoStream == nullptr) {
        LOGE("Cound not find video stream");
        return result;
    }

    seekFrame(formatCtx, videoStream, targetPts);

    int64_t minDeviation = INT64_MAX;
    while (av_read_frame(formatCtx, &packet) >= 0) {
        if (packet.stream_index != videoStream->index) {
            av_packet_unref(&packet);
            continue;
        }
        if (packet.flags & AV_PKT_FLAG_KEY) {
            int64_t currentPts = av_rescale_q(packet.pts, videoStream->time_base, AV_TIME_BASE_Q);
            int64_t deviation =
                    currentPts > targetPts ? currentPts - targetPts : targetPts - currentPts;
            if (deviation < minDeviation && (!backward || currentPts < targetPts)) {
                minDeviation = deviation;
                result = currentPts;
            }
            if (currentPts > targetPts) {
                av_packet_unref(&packet);
                break;
            }
        }
        av_packet_unref(&packet);
    }
    LOGI("get nearly pts for target: %lld, result: %lld", targetPts, result);

    avformat_close_input(&formatCtx);

    return result;
}

int Transcoder::seekFrame(AVFormatContext *formatCtx, AVStream *stream, int64_t timestamp) {
    if (timestamp <= 0) {
        return SUCCEED;
    }
    int64_t timestampTemp = timestamp;
    if (formatCtx->start_time != AV_NOPTS_VALUE) {
        timestamp += formatCtx->start_time;
        if (timestamp < 0) {
            timestamp = timestampTemp;
        }
    }
    timestamp = av_rescale_q(timestamp, AV_TIME_BASE_Q, stream->time_base);
    int ret = av_seek_frame(formatCtx, stream->index, timestamp, AVSEEK_FLAG_BACKWARD);
    if (ret < 0) {
        LOGE("av_seek_frame error: %s， timestamp: %lld, rescale: %lld, duration: %lld",
             av_err2str(ret), timestampTemp, timestamp, formatCtx->duration);
        return FAILED;
    }

    return SUCCEED;
}

int Transcoder::transcode(const char *srcFilePath, const char *dstFilePath, Options *options_) {
    if (srcFilePath == nullptr || dstFilePath == nullptr) {
        LOGE("src & dst file path must be not null");
        return FAILED;
    }

    options = options_ == nullptr ? new Options() : options_;
    if (!options->reencode) {
        int result = transcode(srcFilePath, dstFilePath, options->start, options->duration);
        if (result >= 0) {
            delete options;
            reset();
            return result;
        }
    }

    int ret;
    AVPacket packet = {.data = nullptr, .size = 0};
    AVFrame *frame = nullptr;
    int streamIndex;
    unsigned int i;
    bool endVideo = false;
    bool endAudio = false;
    bool startVideo = false;
    bool startAudio = false;
    int64_t firstVideoTime = -1;

    av_register_all();
    avfilter_register_all();
    av_log_set_callback(androidLog);

    tmpFrame = av_frame_alloc();
    if (!tmpFrame) {
        LOGE("Error allocate frame");
        return FAILED;
    }

    if ((ret = openInputFile(srcFilePath)) < 0)
        goto end;
    if ((ret = openOutputFile(dstFilePath)) < 0)
        goto end;
    if ((ret = initFilters()) < 0)
        goto end;

    if (viStreamIndex != -1) {
        seekFrame(ifmtCtx, ifmtCtx->streams[viStreamIndex], options->start);
    }

    LOGI("Transcoder init succeed, transcoding now(reencoding)...");
    while (1) {
        if ((endVideo || viStreamIndex == -1) && (endAudio || aoStreamIndex == -1)) {
            break;
        }

        if ((ret = av_read_frame(ifmtCtx, &packet)) < 0) {
            LOGI("av_read_frame failed: %s", av_err2str(ret));
            break;
        }

        streamIndex = packet.stream_index;
        AVMediaType mediaType = streamCtx[streamIndex].decodeCtx->codec_type;
        if (mediaType != AVMEDIA_TYPE_VIDEO && mediaType != AVMEDIA_TYPE_AUDIO) {
            av_packet_unref(&packet);
            continue;
        }

        frame = av_frame_alloc();
        if (!frame) {
            LOGE("Error allocate AVFrame");
            av_packet_unref(&packet);
            return FAILED;
        }

        int64_t showTime;
        if ((mediaType == AVMEDIA_TYPE_VIDEO && videoPts == FIRST_VIDEO_PTS)
            || (mediaType == AVMEDIA_TYPE_AUDIO && audioPts == FIRST_AUDIO_PTS)) {
            showTime = av_rescale_q_rnd(packet.pts, ifmtCtx->streams[streamIndex]->time_base,
                                        AV_TIME_BASE_Q,
                                        (AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        } else {
            AVCodecContext *encodeCtx = streamCtx[streamIndex].encodeCtx;
            showTime = mediaType == AVMEDIA_TYPE_VIDEO
                       ? videoPts * AV_TIME_BASE / (int64_t) av_q2d(encodeCtx->framerate)
                       : audioPts * AV_TIME_BASE / encodeCtx->sample_rate;
            showTime += options->start;
        }

        av_packet_rescale_ts(&packet, ifmtCtx->streams[streamIndex]->time_base,
                             streamCtx[streamIndex].decodeCtx->time_base);
        ret = avcodec_send_packet(streamCtx[streamIndex].decodeCtx, &packet);
        if (ret < 0) {
            LOGE("Error sending a packet: %s", av_err2str(ret));
            av_packet_unref(&packet);
            return ret;
        }

        while (ret >= 0) {
            ret = avcodec_receive_frame(streamCtx[streamIndex].decodeCtx, frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            } else if (ret < 0) {
                LOGE("Error during decoding");
                goto end;
            }

            if (options->duration != 0) {
                if (mediaType == AVMEDIA_TYPE_VIDEO) {
                    if ((showTime < (int64_t) (options->start) && !startVideo) || endVideo) {
                        continue;
                    } else if (showTime > (int64_t) (options->start + options->duration)) {
                        endVideo = true;
                    }
                } else {
                    if ((showTime < (int64_t) (options->start) && !startAudio) || !startVideo
                        || endAudio) {
                        continue;
                    } else if (showTime > (int64_t) (options->start + options->duration)) {
                        endAudio = true;
                    }
                }

                if (mediaType == AVMEDIA_TYPE_VIDEO) {
                    startVideo = true;
                } else {
                    startAudio = true;
                }

                if (firstVideoTime == -1) {
                    firstVideoTime = showTime;
                } else if (mediaType == AVMEDIA_TYPE_AUDIO && audioPtsDelay == FIRST_AUDIO_PTS) {
                    int64_t deviation = showTime - firstVideoTime;
                    audioPtsDelay = av_rescale_q(deviation, AV_TIME_BASE_Q,
                                                 streamCtx[streamIndex].encodeCtx->time_base);
                    LOGI("audio first time: %lld, video first time: %lld, audio pts delay: %lld",
                         showTime, firstVideoTime, audioPtsDelay);
                }
            }

            ret = handleEncodeWriteFrame(frame, streamIndex);
            if (ret < 0) {
                goto end;
            }
        }
        av_frame_free(&frame);
        av_packet_unref(&packet);
    }

    while (aoStreamIndex != -1 && audioFifo != nullptr && av_audio_fifo_size(audioFifo) > 0) {
        readAudioToEncode(aoStreamIndex);
    }

    // flush filters and encoders
    for (i = 0; i < ifmtCtx->nb_streams; i++) {
        ret = filterEncodeVideo(nullptr, i);
        if (ret < 0) {
            LOGE("Flushing filter failed");
            goto end;
        }

        ret = flushEncoder(i);
        if (ret < 0) {
            LOGE("Flushing encoder failed");
            goto end;
        }
    }

    av_write_trailer(ofmtCtx);
    LOGI("Transcode succeed!");

    end:
    av_packet_unref(&packet);
    av_frame_free(&frame);
    av_frame_free(&tmpFrame);
    for (i = 0; ifmtCtx != nullptr && i < ifmtCtx->nb_streams; i++) {
        avcodec_free_context(&streamCtx[i].decodeCtx);
        if (ofmtCtx && ofmtCtx->nb_streams > i && ofmtCtx->streams[i] && streamCtx[i].encodeCtx)
            avcodec_free_context(&streamCtx[i].encodeCtx);
        if (filterCtx && filterCtx[i].filterGraph)
            avfilter_graph_free(&filterCtx[i].filterGraph);
    }
    av_free(filterCtx);
    av_free(streamCtx);
    avformat_close_input(&ifmtCtx);
    if (ofmtCtx && !(ofmtCtx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&ofmtCtx->pb);
        avformat_free_context(ofmtCtx);
    }
    sws_freeContext(imgConvertCtx);
    swr_free(&sampleConvertCtx);
    if (audioFifo) {
        av_audio_fifo_free(audioFifo);
    }
    if (tmpFrameBuf) {
        free(tmpFrameBuf);
    }
    delete options;
    reset();

    if (ret < 0) {
        LOGE("Error occurred: %s", av_err2str(ret));
    }

    return ret;
}

int Transcoder::openInputFile(const char *filename) {
    int ret, i;
    ifmtCtx = nullptr;

    if ((ret = avformat_open_input(&ifmtCtx, filename, nullptr, nullptr)) < 0) {
        LOGE("Cannot open input file: %s, ret: %s", filename, av_err2str(ret));
        return ret;
    }

    if ((ret = avformat_find_stream_info(ifmtCtx, nullptr)) < 0) {
        LOGE("Could not find stream info");
        return ret;
    }

    streamCtx = (StreamContext *) av_mallocz_array(ifmtCtx->nb_streams, sizeof(*streamCtx));
    if (!streamCtx) {
        LOGE("Cound not allocate stream context");
        return FAILED;
    }

    AVStream *videoStream = nullptr;

    for (i = 0; i < ifmtCtx->nb_streams; i++) {
        AVStream *stream = ifmtCtx->streams[i];
        AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
        if (!codec) {
            LOGE("Failed to find decoder for stream: #%d", i);
            return FAILED;
        }
        AVCodecContext *codecCtx = avcodec_alloc_context3(codec);
        if (!codecCtx) {
            LOGE("Failed to allocate codec context for stream: #%d", i);
            return FAILED;
        }
        ret = avcodec_parameters_to_context(codecCtx, stream->codecpar);
        if (ret < 0) {
            LOGE("Failed to copy decoder parameters to input decoder context for stream: #%d", i);
            return ret;
        }
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO ||
            stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                totalFrame = getFrameCount(filename);
                codecCtx->framerate = av_guess_frame_rate(ifmtCtx, stream, nullptr);
                codecCtx->time_base = av_inv_q(codecCtx->framerate);
                viStreamIndex = stream->index;
            }
            if ((ret = avcodec_open2(codecCtx, codec, nullptr)) < 0) {
                LOGE("Failed to open codec for stream: %d", i);
                return ret;
            }
        }
        streamCtx[i].decodeCtx = codecCtx;
    }
    av_dump_format(ifmtCtx, 0, filename, 0);

    return SUCCEED;
}

int getFrameCount(const char *filePath) {
    AVFormatContext *formatCtx = nullptr;
    AVStream *videoStream = nullptr;
    AVPacket packet = {.size = 0, .data = nullptr};
    int frameCount = 0;

    av_register_all();

    if (avformat_open_input(&formatCtx, filePath, 0, 0) < 0) {
        LOGE("Could not open input file '%s'", filePath);
        return frameCount;
    }

    if (avformat_find_stream_info(formatCtx, 0) < 0) {
        LOGE("Could not open input file '%s'", filePath);
        avformat_close_input(&formatCtx);
        return frameCount;
    }

    av_dump_format(formatCtx, 0, filePath, 0);

    for (int i = 0; i < formatCtx->nb_streams; ++i) {
        if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = formatCtx->streams[i];
            break;
        }
    }

    if (videoStream == nullptr) {
        LOGE("Cound not find video stream");
        avformat_close_input(&formatCtx);
        return frameCount;
    }

    while (av_read_frame(formatCtx, &packet) >= 0) {
        if (packet.stream_index != videoStream->index) {
            av_packet_unref(&packet);
            continue;
        }
        frameCount++;
        av_packet_unref(&packet);
    }

    avformat_close_input(&formatCtx);

    return frameCount;
}

int Transcoder::openOutputFile(const char *filename) {
    AVStream *outStream, *inStream;
    AVCodecContext *decodeCtx, *encodeCtx;
    AVCodec *encoder;
    int ret, i;

    ofmtCtx = nullptr;
    avformat_alloc_output_context2(&ofmtCtx, nullptr, nullptr, filename);
    if (!ofmtCtx) {
        LOGI("Could not create output context");
        return AVERROR_UNKNOWN;
    }

    for (i = 0; i < ifmtCtx->nb_streams; i++) {
        outStream = avformat_new_stream(ofmtCtx, nullptr);
        if (!outStream) {
            LOGE("Failed allocating output stream");
            return AVERROR_UNKNOWN;
        }

        inStream = ifmtCtx->streams[i];
        decodeCtx = streamCtx[i].decodeCtx;

        if (decodeCtx->codec_type == AVMEDIA_TYPE_VIDEO ||
            decodeCtx->codec_type == AVMEDIA_TYPE_AUDIO) {
            if (decodeCtx->codec_type == AVMEDIA_TYPE_VIDEO) {
                encoder = avcodec_find_encoder(AV_CODEC_ID_H264);
            } else {
                encoder = avcodec_find_encoder(AV_CODEC_ID_AAC);
            }
            if (!encoder) {
                LOGE("Necessary encoder not found");
                return AVERROR_INVALIDDATA;
            }
            encodeCtx = avcodec_alloc_context3(encoder);
            if (!encodeCtx) {
                LOGE("Failed to allocate the encoder context");
                return AVERROR(ENOMEM);
            }

            if (decodeCtx->codec_type == AVMEDIA_TYPE_VIDEO) {
                setEncoderSize(encodeCtx, decodeCtx->width, decodeCtx->height);
                encodeCtx->sample_aspect_ratio = decodeCtx->sample_aspect_ratio;
                encodeCtx->pix_fmt = AV_PIX_FMT_YUV420P;
                if (options->maxBitRate > 0) {
                    encodeCtx->rc_max_rate = options->maxBitRate;
                    encodeCtx->rc_buffer_size = (int) options->maxBitRate * 2;
                } else {
                    encodeCtx->rc_max_rate = ifmtCtx->bit_rate * 3 / 2;
                    encodeCtx->rc_buffer_size = (int) encodeCtx->rc_max_rate * 2;
                }
                encodeCtx->framerate = av_d2q(1.0f * totalFrame * AV_TIME_BASE / ifmtCtx->duration,
                                              1000000);
                encodeCtx->time_base = av_inv_q(encodeCtx->framerate);
                encodeCtx->gop_size = (int) av_q2d(encodeCtx->framerate) * 2;

                int size = av_image_get_buffer_size(encodeCtx->pix_fmt, encodeCtx->width,
                                                    encodeCtx->height, 1);
                if (size < 0) {
                    LOGE("image get buffer size error: %s", av_err2str(size));
                    return FAILED;
                }
                tmpFrameBuf = (uint8_t *) malloc((size_t) size);
                if (tmpFrameBuf == nullptr) {
                    LOGE("Could not allocate memory");
                    return FAILED;
                }
            } else {
                encodeCtx->sample_rate = decodeCtx->sample_rate;
                encodeCtx->channel_layout = decodeCtx->channel_layout;
                encodeCtx->channels = av_get_channel_layout_nb_channels(encodeCtx->channel_layout);
                // take first format from list of supported formats
                encodeCtx->sample_fmt = encoder->sample_fmts[0];
                encodeCtx->time_base = AVRational{1, encodeCtx->sample_rate};

                audioFifo = av_audio_fifo_alloc(encodeCtx->sample_fmt, encodeCtx->channels, 1);
                if (audioFifo == nullptr) {
                    LOGE("av_audio_fifo_alloc failed");
                    return FAILED;
                }

                sampleConvertCtx = swr_alloc_set_opts(sampleConvertCtx,
                                                      encodeCtx->channel_layout,
                                                      encodeCtx->sample_fmt,
                                                      encodeCtx->sample_rate,
                                                      av_get_default_channel_layout(
                                                              decodeCtx->channels),
                                                      decodeCtx->sample_fmt, decodeCtx->sample_rate,
                                                      0, nullptr);
                if (!sampleConvertCtx) {
                    LOGE("sampleConvertCtx alloc failed, audio record may be failed!");
                } else if (swr_init(sampleConvertCtx) < 0) {
                    LOGE("sampleConvertCtx init failed, audio record may be failed!");
                }
                aoStreamIndex = outStream->index;
            }

            if (ofmtCtx->oformat->flags & AVFMT_GLOBALHEADER) {
                encodeCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
            }

            AVDictionary *opts = nullptr;
            map<string, string>::iterator it = options->encodeOptions.begin();
            for (; it != options->encodeOptions.end(); it++) {
                av_dict_set(&opts, it->first.c_str(), it->second.c_str(), 0);
            }
            if ((ret = avcodec_open2(encodeCtx, encoder, &opts)) < 0) {
                LOGE("Cannot open video encoder for stream #%d", i);
                av_dict_free(&opts);
                return ret;
            }
            av_dict_free(&opts);

            ret = avcodec_parameters_from_context(outStream->codecpar, encodeCtx);
            if (ret < 0) {
                LOGE("Failed to copy encoder parameters to output stream #%d", i);
                return ret;
            }

            outStream->time_base = encodeCtx->time_base;
            streamCtx[i].encodeCtx = encodeCtx;
        } else if (decodeCtx->codec_type == AVMEDIA_TYPE_UNKNOWN) {
            LOGE("Elementary stream #%d is of unknown type, cannot proceed", i);
            return AVERROR_INVALIDDATA;
        } else {
            // if this stream must be remuxed
            if ((ret = avcodec_parameters_copy(outStream->codecpar, inStream->codecpar)) < 0) {
                LOGE("Copying parameters for stream #%d failed", i);
                return ret;
            }
            outStream->time_base = inStream->time_base;
        }
    }
    av_dump_format(ofmtCtx, 0, filename, 1);

    if (!(ofmtCtx->oformat->flags & AVFMT_NOFILE)) {
        if ((ret = avio_open(&ofmtCtx->pb, filename, AVIO_FLAG_WRITE)) < 0) {
            LOGE("Could not open output file '%s'", filename);
            return ret;
        }
    }

    // init muxer, write output file header
    ret = avformat_write_header(ofmtCtx, nullptr);
    if (ret < 0) {
        LOGE("Error occurred when opening output file");
        return ret;
    }

    return SUCCEED;
}

void setEncoderSize(AVCodecContext *codecCtx, int width, int height) {
    if (width % 2 == 1) {
        if (width < height) {
            height = (int) (1.0 * (width - 1) / width * height);
            height = height % 2 == 0 ? height : height - 1;
        }
        width--;
    }
    if (height % 2 == 1) {
        if (height < width) {
            width = (int) (1.0 * (height - 1) / height * width);
            width = width % 2 == 0 ? width : width - 1;
        }
        height--;
    }
    codecCtx->width = width;
    codecCtx->height = height;
}

int Transcoder::initFilters() {

    const char *filterSpec;
    int ret, i;
    filterCtx = (FilteringContext *) av_malloc_array(ifmtCtx->nb_streams, sizeof(*filterCtx));
    if (!filterCtx) {
        return AVERROR(ENOMEM);
    }

    for (i = 0; i < ifmtCtx->nb_streams; i++) {
        filterCtx[i].buffersrcCtx = nullptr;
        filterCtx[i].buffersinkCtx = nullptr;
        filterCtx[i].filterGraph = nullptr;
        if (!(ifmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO
              || ifmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)) {
            continue;
        }

        if (ifmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            filterSpec = options->videoFilter;
        else
            filterSpec = options->audioFilter;
        ret = initFilters(&filterCtx[i], streamCtx[i].decodeCtx, streamCtx[i].encodeCtx,
                          filterSpec);
        if (ret) {
            return ret;
        }
    }

    return SUCCEED;
}

int Transcoder::initFilters(FilteringContext *filterCtx, AVCodecContext *decodeCtx,
                            AVCodecContext *encodeCtx, const char *filterSpec) {
    char args[512];
    int ret = 0;
    AVFilter *buffersrc = nullptr;
    AVFilter *buffersink = nullptr;
    AVFilterContext *buffersrcCtx = nullptr;
    AVFilterContext *buffersinkCtx = nullptr;
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs = avfilter_inout_alloc();
    AVFilterGraph *filterGraph = avfilter_graph_alloc();

    if (!outputs || !inputs || !filterGraph) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    if (decodeCtx->codec_type == AVMEDIA_TYPE_VIDEO) {
        buffersrc = avfilter_get_by_name("buffer");
        buffersink = avfilter_get_by_name("buffersink");
        if (!buffersrc || !buffersink) {
            LOGE("filtering source or sink element not found");
            ret = AVERROR_UNKNOWN;
            goto end;
        }

        snprintf(args, sizeof(args),
                 "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
                 decodeCtx->width, decodeCtx->height, decodeCtx->pix_fmt, decodeCtx->time_base.num,
                 decodeCtx->time_base.den, decodeCtx->sample_aspect_ratio.num,
                 decodeCtx->sample_aspect_ratio.den);
        LOGI("video filter:\n%s\n%s", args, filterSpec);

        ret = avfilter_graph_create_filter(&buffersrcCtx, buffersrc, "in",
                                           args, nullptr, filterGraph);
        if (ret < 0) {
            LOGE("Cannot create buffer source");
            goto end;
        }

        ret = avfilter_graph_create_filter(&buffersinkCtx, buffersink, "out",
                                           nullptr, nullptr, filterGraph);
        if (ret < 0) {
            LOGE("Cannot create buffer sink");
            goto end;
        }

        // 用于转换 pixel format
        ret = av_opt_set_bin(buffersinkCtx, "pix_fmts",
                             (uint8_t *) &encodeCtx->pix_fmt, sizeof(encodeCtx->pix_fmt),
                             AV_OPT_SEARCH_CHILDREN);
        if (ret < 0) {
            LOGE("Cannot set output pixel format");
            goto end;
        }

    } else if (decodeCtx->codec_type == AVMEDIA_TYPE_AUDIO) {
        buffersrc = avfilter_get_by_name("abuffer");
        buffersink = avfilter_get_by_name("abuffersink");
        if (!buffersrc || !buffersink) {
            LOGE("filtering source or sink element not found");
            ret = AVERROR_UNKNOWN;
            goto end;
        }

        if (!decodeCtx->channel_layout) {
            decodeCtx->channel_layout =
                    (uint64_t) av_get_default_channel_layout(decodeCtx->channels);
        }

        // 在 fifo audio frame 的过程中，audio frame 的格式已被转换
        snprintf(args, sizeof(args),
                 "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=%lld",
                 encodeCtx->time_base.num, encodeCtx->time_base.den, encodeCtx->sample_rate,
                 av_get_sample_fmt_name(encodeCtx->sample_fmt), encodeCtx->channel_layout);
        LOGI("audio filter:\n%s\n%s", args, filterSpec);

        ret = avfilter_graph_create_filter(&buffersrcCtx, buffersrc, "in",
                                           args, nullptr, filterGraph);
        if (ret < 0) {
            LOGE("Cannot create audio buffer source");
            goto end;
        }

        ret = avfilter_graph_create_filter(&buffersinkCtx, buffersink, "out",
                                           nullptr, nullptr, filterGraph);
        if (ret < 0) {
            LOGE("Cannot create audio buffer sink");
            goto end;
        }
    } else {
        ret = AVERROR_UNKNOWN;
        goto end;
    }

    // Endpoints for the filter graph.
    outputs->name = av_strdup("in");
    outputs->filter_ctx = buffersrcCtx;
    outputs->pad_idx = 0;
    outputs->next = nullptr;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = buffersinkCtx;
    inputs->pad_idx = 0;
    inputs->next = nullptr;

    if (!outputs->name || !inputs->name) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    if ((ret = avfilter_graph_parse_ptr(filterGraph, filterSpec,
                                        &inputs, &outputs, nullptr)) < 0)
        goto end;

    if ((ret = avfilter_graph_config(filterGraph, nullptr)) < 0)
        goto end;

    filterCtx->buffersrcCtx = buffersrcCtx;
    filterCtx->buffersinkCtx = buffersinkCtx;
    filterCtx->filterGraph = filterGraph;

    end:
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);

    return ret;
}

int Transcoder::handleEncodeWriteFrame(AVFrame *frame, int streamIndex) {
    AVMediaType mediaType = streamCtx[streamIndex].encodeCtx->codec_type;
    int ret = 0;
    if (mediaType == AVMEDIA_TYPE_VIDEO) {
        ret = filterEncodeVideo(frame, streamIndex);
    } else if (mediaType == AVMEDIA_TYPE_AUDIO) {
        ret = encodeAudioWidthFifo(frame, streamIndex);
    }
    return ret;
}

int Transcoder::filterEncodeVideo(AVFrame *frame, int streamIndex) {
    int ret;
    AVFrame *filtFrame;

    // push the decoded frame into the filtergraph
    ret = av_buffersrc_add_frame_flags(filterCtx[streamIndex].buffersrcCtx, frame, 0);
    if (ret < 0) {
        LOGE("Error while feeding the filtergraph");
        return ret;
    }

    // pull filtered frames from the filtergraph
    while (1) {
        filtFrame = av_frame_alloc();
        if (!filtFrame) {
            ret = AVERROR(ENOMEM);
            break;
        }
        ret = av_buffersink_get_frame(filterCtx[streamIndex].buffersinkCtx, filtFrame);
        if (ret < 0) {
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                ret = 0;
            av_frame_free(&filtFrame);
            break;
        }

        filtFrame->pict_type = AV_PICTURE_TYPE_NONE;
        ret = encodeAudioOrVideo(filtFrame, streamIndex);
        av_frame_free(&filtFrame);
        if (ret < 0) {
            break;
        }
    }

    return ret;
}

int Transcoder::encodeAudioOrVideo(AVFrame *frame, int streamIndex) {
    AVMediaType mediaType = streamCtx[streamIndex].encodeCtx->codec_type;
    int ret = 0;
    if (mediaType == AVMEDIA_TYPE_VIDEO) {
        ret = rescaleVideoToEncode(frame, streamIndex);
    } else if (mediaType == AVMEDIA_TYPE_AUDIO) {
        ret = encodeAudioWidthFifo(frame, streamIndex);
    }
    return ret;
}

int Transcoder::rescaleVideoToEncode(AVFrame *frame, int streamIndex) {
    videoPts = videoPts == FIRST_VIDEO_PTS ? 0 : videoPts + 1;
    frame->pts = videoPts;
    bool change = false;
    AVCodecContext *codecCtx = streamCtx[streamIndex].encodeCtx;
    int width = codecCtx->width;
    int height = codecCtx->height;
    AVPixelFormat pixelFormat = codecCtx->pix_fmt;
    if (codecCtx->codec_type == AVMEDIA_TYPE_VIDEO && frame != nullptr &&
        (width != frame->width || height != frame->height ||
         pixelFormat != AVPixelFormat(frame->format))) {
        imgConvertCtx = sws_getCachedContext(imgConvertCtx, frame->width, frame->height,
                                             AVPixelFormat(frame->format), width, height,
                                             pixelFormat, SWS_BILINEAR, nullptr, nullptr, nullptr);
        if (!imgConvertCtx) {
            LOGE("sws_getCachedContext error");
            return FAILED;
        }
        av_image_fill_arrays(tmpFrame->data, tmpFrame->linesize, tmpFrameBuf, pixelFormat, width,
                             height, 1);
        tmpFrame->format = pixelFormat;
        tmpFrame->width = width;
        tmpFrame->height = height;
        tmpFrame->pts = frame->pts;
        sws_scale(imgConvertCtx, (const uint8_t *const *) frame->data, frame->linesize, 0,
                  frame->height, tmpFrame->data, tmpFrame->linesize);
        change = true;
    }
    return change
           ? doEncodeWriteFrame(tmpFrame, streamIndex, nullptr)
           : doEncodeWriteFrame(frame, streamIndex, nullptr);
}

int Transcoder::encodeAudioWidthFifo(AVFrame *frame, int streamIndex) {
    int ret;
    AVCodecContext *codecCtx = streamCtx[streamIndex].encodeCtx;
    if (frame != nullptr) {
        ret = storeAudioFrame(frame, codecCtx);
        if (ret < 0) {
            LOGE("storeAudioFrame failed: %s", av_err2str(ret));
            return ret;
        }

        while (av_audio_fifo_size(audioFifo) >= codecCtx->frame_size) {
            if ((ret = readAudioToEncode(streamIndex)) < 0) {
                LOGE("readAudioToEncode failed: %s", av_err2str(ret));
                break;
            }
        }
    } else {
        if (av_audio_fifo_size(audioFifo) > 0) {
            if ((ret = readAudioToEncode(streamIndex)) < 0) {
                LOGE("readAudioToEncode failed: %s", av_err2str(ret));
            }
        } else {
            ret = doEncodeWriteFrame(nullptr, streamIndex, nullptr);
            if (ret < 0) {
                LOGE("doEncodeWriteFrame failed: %s", av_err2str(ret));
            }
        }
    }

    return SUCCEED;
}

int Transcoder::storeAudioFrame(AVFrame *inputFrame, AVCodecContext *outCodecCtx) {
    int ret;
    uint8_t **convertedSamples = nullptr;

    int channels = outCodecCtx->channels;
    int frameSize = inputFrame->nb_samples;
    if (!(convertedSamples = (uint8_t **) calloc((size_t) channels, sizeof(*convertedSamples)))) {
        LOGE("Could not allocate converted sample pointers");
        return AVERROR(ENOMEM);
    }

    if ((ret = av_samples_alloc(convertedSamples, nullptr, channels, frameSize,
                                outCodecCtx->sample_fmt, 0)) < 0) {
        LOGE("Could not allocate converted input samples (error '%s')\n", av_err2str(ret));
        goto cleanup;
    }

    if ((ret = swr_convert(sampleConvertCtx, convertedSamples, frameSize,
                           (const uint8_t **) inputFrame->extended_data, frameSize)) < 0) {
        LOGE("Could not convert input samples (error '%s')", av_err2str(ret));
        goto cleanup;
    }

    if ((ret = av_audio_fifo_realloc(audioFifo, av_audio_fifo_size(audioFifo) + frameSize)) < 0) {
        LOGE("Could not reallocate FIFO");
        goto cleanup;
    }

    if (av_audio_fifo_write(audioFifo, (void **) convertedSamples, frameSize) < frameSize) {
        LOGE("Could not write data to FIFO");
        ret = AVERROR_EXIT;
        goto cleanup;
    }

    ret = SUCCEED;

    cleanup:
    if (convertedSamples) {
        av_freep(&convertedSamples[0]);
        free(convertedSamples);
    }

    return ret;
}

int Transcoder::readAudioToEncode(int streamIndex) {
    int ret;
    AVCodecContext *codecCtx = streamCtx[streamIndex].encodeCtx;

    AVFrame *outputFrame;
    const int frameSize = FFMIN(av_audio_fifo_size(audioFifo), codecCtx->frame_size);
    if (initAudioOutputFrame(&outputFrame, codecCtx, frameSize) < 0)
        return AVERROR_EXIT;

    audioPtsDelay = audioPtsDelay == FIRST_AUDIO_PTS ? 0 : audioPtsDelay;
    audioPts = audioPts == FIRST_AUDIO_PTS ? 0 : audioPts + outputFrame->nb_samples;

    if (av_audio_fifo_read(audioFifo, (void **) outputFrame->data, frameSize) < frameSize) {
        LOGE("Could not read data from FIFO");
        av_frame_free(&outputFrame);
        return AVERROR_EXIT;
    }

    outputFrame->pts = audioPts;
    ret = doEncodeWriteFrame(outputFrame, streamIndex, nullptr);
    av_frame_free(&outputFrame);

    if (ret < 0) {
        LOGE("doEncodeWriteFrame error: %s", av_err2str(ret));
        return ret;
    }

    return SUCCEED;
}

int Transcoder::initAudioOutputFrame(AVFrame **frame, AVCodecContext *outCodecCtx, int frameSize) {
    int error;

    if (!(*frame = av_frame_alloc())) {
        LOGE("Could not allocate output frame\n");
        return AVERROR_EXIT;
    }

    (*frame)->nb_samples = frameSize;
    (*frame)->channel_layout = outCodecCtx->channel_layout;
    (*frame)->format = outCodecCtx->sample_fmt;
    (*frame)->sample_rate = outCodecCtx->sample_rate;

    if ((error = av_frame_get_buffer(*frame, 0)) < 0) {
        LOGE("Could not allocate output frame samples (error '%s')", av_err2str(error));
        av_frame_free(frame);
        return error;
    }

    return SUCCEED;
}

int Transcoder::doEncodeWriteFrame(AVFrame *frame, int streamIndex, int *gotFrame) {
    int ret;
    int gotFrameLocal;
    if (!gotFrame)
        gotFrame = &gotFrameLocal;
    *gotFrame = 0;

    AVPacket *encodePkt = av_packet_alloc();
    AVCodecContext *codecCtx = streamCtx[streamIndex].encodeCtx;
    const char *type = codecCtx->codec_type == AVMEDIA_TYPE_VIDEO ? "video" : "audio";
    if (!encodePkt) {
        LOGE("Error allocate %s encode packet", type);
        return FAILED;
    }

    ret = avcodec_send_frame(codecCtx, frame);
    if (ret < 0) {
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return 0;
        }
        LOGE("Error sending a %s frame for encoding: %s", type, av_err2str(ret));
        return ret;
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(codecCtx, encodePkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            LOGE("Error during encoding %s", type);
            return FAILED;
        }

        av_packet_rescale_ts(encodePkt, codecCtx->time_base,
                             ofmtCtx->streams[streamIndex]->time_base);
        encodePkt->stream_index = streamIndex;

        ret = av_interleaved_write_frame(ofmtCtx, encodePkt);
        if (ret < 0) {
            LOGE("Error during muxing %s frame: %s", type, av_err2str(ret));
            return ret;
        }
        *gotFrame = 1;
    }

    return SUCCEED;
}

int Transcoder::flushEncoder(int streamIndex) {
    int ret;
    int gotFrame;

    if (!(streamCtx[streamIndex].encodeCtx->codec->capabilities & AV_CODEC_CAP_DELAY))
        return 0;

    LOGI("Flushing stream #%u encoder", streamIndex);
    while (1) {
        ret = doEncodeWriteFrame(nullptr, streamIndex, &gotFrame);
        if (ret < 0)
            break;
        if (!gotFrame)
            return 0;
    }
    return ret;
}
