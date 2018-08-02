package com.steven.avgraphics.module.av;



@SuppressWarnings("WeakerAccess")
public class AVInfo {

    // video
    public String vCodec;
    public long vDuration;
    public int vBitRate;
    public int width;
    public int height;
    public int frameRate;
    public int colorFormat;

    // audio
    public String aCodec;
    public long aDuration;
    public int aBitRate;
    public int sampleRate;
    public int channels;
    public int sampleFormat;

    @Override
    public String toString() {
        return "{video: [codec: " + vCodec + "], " +
                "[duration: " + vDuration + "], " +
                "[bitrate: " + vBitRate + "], " +
                "[width: " + width + "], " +
                "[height: " + height + "], " +
                "[frameRate: " + frameRate + "], " +
                "[colorFormat: " + colorFormat + "]" +
                "}\n" +
                "{audio: [codec: " + aCodec + "], " +
                "[duration: " + aDuration + "], " +
                "[bitrate: " + aBitRate + "], " +
                "[sampleRate: " + sampleRate + "], " +
                "[channels: " + channels + "], " +
                "[sampleFormat: " + sampleFormat + "]" +
                "}";
    }



}
