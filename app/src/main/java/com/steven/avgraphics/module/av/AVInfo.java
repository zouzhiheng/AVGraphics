package com.steven.avgraphics.module.av;



@SuppressWarnings("WeakerAccess")
public class AVInfo {

    public static final int VCODEC_UNKNOWN = 0;
    public static final int VCODEC_H264 = 1;
    public static final int VCODEC_MPEG = 2;
    public static final int VCODEC_H265 = 3;
    public static final int VCODEC_OTHER = 4;

    public static final int ACODEC_UNKNOWN = 0;
    public static final int ACODEC_AAC = 1;
    public static final int ACODEC_MP3 = 2;
    public static final int ACODEC_OTHER = 3;

    public static final int PIXEL_FORMAT_NONE = 0;
    public static final int PIXEL_FORMAT_NV21 = 1;
    public static final int PIXEL_FORMAT_YV12 = 2;
    public static final int PIXEL_FORMAT_NV12 = 3;
    public static final int PIXEL_FORMAT_YUV420P = 4;

    public static final int SAMPLE_FORMAT_NONE = 0;
    public static final int SAMPLE_FORMAT_8BIT = 8;
    public static final int SAMPLE_FORMAT_16BIT = 16;
    public static final int SAMPLE_FORMAT_FLOAT = 32;

    public boolean isHaveVideo;
    public int vcodec;
    public int width;
    public int height;
    public int frameRate;
    public int pixelFormat;

    public boolean isHaveAudio;
    public int acodec;
    public int channels;
    public int sampleRate;
    public int sampleFormat;

    public int bitRate;
    public long duration;

    public AVInfo() {
    }

    public String getVideoCodecName() {
        String name = "unknown";
        switch (vcodec) {
            case VCODEC_H264:
                name = "h264";
                break;
            case VCODEC_H265:
                name = "h265";
                break;
            case VCODEC_MPEG:
                name = "mpeg";
                break;
            case VCODEC_OTHER:
                name = "other";
                break;
        }
        return name;
    }

    public String getAudioCodecName() {
        String name = "unknown";
        switch (acodec) {
            case ACODEC_AAC:
                name = "aac";
                break;
            case ACODEC_MP3:
                name = "mp3";
                break;
            case ACODEC_OTHER:
                name = "other";
                break;
        }
        return name;
    }

    public String getPixelFormatName() {
        String name = "unknown";
        switch (pixelFormat) {
            case PIXEL_FORMAT_NV12:
                name = "nv12";
                break;
            case PIXEL_FORMAT_NV21:
                name = "nv21";
                break;
            case PIXEL_FORMAT_YV12:
                name = "yv12";
                break;
            case PIXEL_FORMAT_YUV420P:
                name = "yuv420p";
                break;
        }
        return name;
    }

    public String getSampleFormatName() {
        String name = "unknown";
        switch (sampleFormat) {
            case SAMPLE_FORMAT_8BIT:
                name = "s8";
                break;
            case SAMPLE_FORMAT_16BIT:
                name = "s16";
                break;
            case SAMPLE_FORMAT_FLOAT:
                name = "float";
                break;
        }
        return name;
    }

    @Override
    public String toString() {
        return  "[isHaveVideo: " + isHaveVideo + "], "
                + "[vcodec: " + getVideoCodecName() + "], "
                + "[width: " + width + "], "
                + "[height: " + height + "], "
                + "[frameRate: " + frameRate + "], "
                + "[pixelFormat: " + getPixelFormatName() + "]"
                + "\n"
                + "[isHaveAudio: " + isHaveAudio + "], "
                + "[acodec: " + getAudioCodecName() + "], "
                + "[channels: " + channels + "], "
                + "[sampleRate: " + sampleRate + "], "
                + "[sampleFormat: " + getSampleFormatName() + "]"
                + "\n"
                + "[bitRate: " + bitRate + "], "
                + "[duration: " + duration / 1000 / 1000 + "s]";
    }

}
