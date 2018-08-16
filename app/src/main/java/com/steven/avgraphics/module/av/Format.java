package com.steven.avgraphics.module.av;


import android.media.AudioFormat;
import android.os.Build;

public class Format {

    public static final int PIXEL_FORMAT_NONE = 0;
    public static final int PIXEL_FORMAT_NV21 = 1;
    public static final int PIXEL_FORMAT_YV12 = 2;
    public static final int PIXEL_FORMAT_NV12 = 3;
    public static final int PIXEL_FORMAT_YUV420P = 4;

    public static final int SAMPLE_FORMAT_NONE = 0;
    public static final int SAMPLE_FORMAT_8BIT = 8;
    public static final int SAMPLE_FORMAT_16BIT = 16;
    public static final int SAMPLE_FORMAT_FLOAT = 24;

    public static String getPixelFormatName(int pixelFormat) {
        String name = "unkonwn";
        switch (pixelFormat) {
            case PIXEL_FORMAT_NV12:
                name = "nv12";
                break;
            case PIXEL_FORMAT_NV21:
                name = "nv21";
                break;
            case PIXEL_FORMAT_YUV420P:
                name = "yuv420p";
                break;
            case PIXEL_FORMAT_YV12:
                name = "yv12";
                break;
        }
        return name;
    }

    public static String getSampleFormatName(int sampleFormat) {
        String name = "unkonwn";
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

    public static int getAudioFormat(int sampleFormat) {
        int audioFormat = AudioFormat.ENCODING_PCM_16BIT;
        if (sampleFormat == SAMPLE_FORMAT_8BIT) {
            audioFormat = AudioFormat.ENCODING_PCM_8BIT;
        } else if (sampleFormat == SAMPLE_FORMAT_FLOAT
                && Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            audioFormat = AudioFormat.ENCODING_PCM_FLOAT;
        }
        return audioFormat;
    }


}
