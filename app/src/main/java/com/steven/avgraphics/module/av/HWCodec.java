package com.steven.avgraphics.module.av;


import android.media.MediaCodec;
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.media.MediaMuxer;
import android.os.Build;
import android.util.Log;
import android.util.SparseIntArray;

import java.io.IOException;
import java.nio.ByteBuffer;

@SuppressWarnings("WeakerAccess")
public class HWCodec {

    private static final String TAG = "HWCodec";

    public static final int MEDIA_TYPE_VIDEO = 1;
    public static final int MEDIA_TYPE_AUDIO = 2;
    public static final int MEDIA_TYPE_UNKNOWN = 0;

    public static final String MIME_TYPE_AVC = "video/avc";
    public static final String MIME_TYPE_AAC = "audio/mp4a-latm";

    public static AVInfo getAVInfo(String filePath) {
        MediaExtractor extractor = new MediaExtractor();
        try {
            extractor.setDataSource(filePath);
        } catch (IOException e) {
            e.printStackTrace();
            return null;
        }

        AVInfo info = new AVInfo();
        for (int i = 0; i < extractor.getTrackCount(); i++) {
            MediaFormat format = extractor.getTrackFormat(i);
            if (getMediaType(format) == MEDIA_TYPE_VIDEO) {
                info.vBitRate = getFormatInt(format, MediaFormat.KEY_BIT_RATE);
                info.vDuration = getFormatLong(format, MediaFormat.KEY_DURATION);
                info.vCodec = getFormatString(format, MediaFormat.KEY_MIME);
                info.width = getFormatInt(format, MediaFormat.KEY_WIDTH);
                info.height = getFormatInt(format, MediaFormat.KEY_HEIGHT);
                info.frameRate = getFormatInt(format, MediaFormat.KEY_FRAME_RATE);
                info.colorFormat = getFormatInt(format, MediaFormat.KEY_COLOR_FORMAT);
            } else if (getMediaType(format) == MEDIA_TYPE_AUDIO) {
                info.aBitRate = getFormatInt(format, MediaFormat.KEY_BIT_RATE);
                info.aDuration = getFormatLong(format, MediaFormat.KEY_DURATION);
                info.aCodec = getFormatString(format, MediaFormat.KEY_MIME);
                info.channels = getFormatInt(format, MediaFormat.KEY_CHANNEL_COUNT);
                info.sampleRate = getFormatInt(format, MediaFormat.KEY_SAMPLE_RATE);
            }
        }

        return info;
    }

    private static int getFormatInt(MediaFormat format, String name) {
        int result;
        try {
            result = format.getInteger(name);
        } catch (Exception e) {
            result = -1;
        }
        return result;
    }

    private static long getFormatLong(MediaFormat format, String name) {
        long result;
        try {
            result = format.getLong(name);
        } catch (Exception e) {
            result = -1;
        }
        return result;
    }

    private static String getFormatString(MediaFormat format, String name) {
        String result;
        try {
            result = format.getString(name);
        } catch (Exception e) {
            result = "null";
        }
        return result;
    }

    public static int getMediaType(MediaFormat format) {
        String mime = format.getString(MediaFormat.KEY_MIME);
        if (mime.startsWith("video/")) {
            return MEDIA_TYPE_VIDEO;
        } else if (mime.startsWith("audio/")) {
            return MEDIA_TYPE_AUDIO;
        }
        return MEDIA_TYPE_UNKNOWN;
    }

    public static boolean transcode(String srcFilePath, String dstFilePath) {
        MediaExtractor extractor = null;
        MediaMuxer muxer = null;
        try {
            extractor = new MediaExtractor();
            extractor.setDataSource(srcFilePath);
            muxer = new MediaMuxer(dstFilePath, MediaMuxer.OutputFormat.MUXER_OUTPUT_MPEG_4);
            doTranscode(extractor, muxer);
        } catch (IOException e) {
            Log.e(TAG, "doTranscode io exception: " + e.getLocalizedMessage());
            return false;
        } catch (Exception e) {
            Log.e(TAG, "doTranscode exception: " + e.getLocalizedMessage());
            return false;
        } finally {
            try {
                if (extractor != null) {
                    extractor.release();
                }

                if (muxer != null) {
                    muxer.stop();
                    muxer.release();
                }
            } catch (Exception e) {
                Log.e(TAG, "doTranscode close exception: " + e.getLocalizedMessage());
            }
        }
        return true;
    }

    private static void doTranscode(MediaExtractor extractor, MediaMuxer muxer) throws IOException {
        int trackCount = extractor.getTrackCount();
        SparseIntArray trackMap = new SparseIntArray(trackCount);
        for (int i = 0; i < trackCount; ++i) {
            MediaFormat format = extractor.getTrackFormat(i);
            if (getMediaType(format) == MEDIA_TYPE_UNKNOWN) {
                trackMap.put(i, -1);
            } else {
                int trackIndex = muxer.addTrack(format);
                trackMap.put(i, trackIndex);
            }
        }

        muxer.start();
        for (int i = 0; i < trackCount; ++i) {
            int trackIndex = trackMap.get(i);
            if (trackIndex == -1) {
                continue;
            }
            extractor.selectTrack(i);

            MediaFormat format = extractor.getTrackFormat(i);
            int maxBufferSize = format.getInteger(MediaFormat.KEY_MAX_INPUT_SIZE);
            long timeUnit = 0;
            boolean isVideo = getMediaType(format) == MEDIA_TYPE_VIDEO;
            if (isVideo) {
                int framerate = format.getInteger(MediaFormat.KEY_FRAME_RATE);
                timeUnit = 1000 * 1000 / framerate;
            }
            ByteBuffer source = ByteBuffer.allocate(maxBufferSize);
            MediaCodec.BufferInfo bufferInfo = new MediaCodec.BufferInfo();

            int size;
            while ((size = extractor.readSampleData(source, 0)) >= 0) {
                bufferInfo.offset = 0;
                bufferInfo.size = size;
                bufferInfo.flags = extractor.getSampleFlags();
                // api 24 以上可以封装 b 帧，之前版本的视频帧的 pts 必须是递增的
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N || !isVideo) {
                    bufferInfo.presentationTimeUs = extractor.getSampleTime();
                } else {
                    bufferInfo.presentationTimeUs += timeUnit;
                }
                muxer.writeSampleData(trackIndex, source, bufferInfo);
                extractor.advance();
            }
            extractor.unselectTrack(i);
        }
    }


}
