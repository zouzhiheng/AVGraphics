package com.steven.avgraphics.module.av;


import android.media.MediaCodec;
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.media.MediaMuxer;
import android.os.Build;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.util.Log;
import android.util.SparseIntArray;
import android.view.Surface;

import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;

@SuppressWarnings("WeakerAccess")
public class HWCodec {

    private static final String TAG = "HWCodec";
    private static final long DEFAULT_TIMEOUT = 10 * 1000;

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

    public static boolean decode(String srcFilePath, String yuvDst, String pcmDst) {
        boolean vsucceed = doDecode(srcFilePath, yuvDst, MEDIA_TYPE_VIDEO);
        boolean asucceed = doDecode(srcFilePath, pcmDst, MEDIA_TYPE_AUDIO);
        return vsucceed && asucceed;
    }

    public static boolean decode(@NonNull String srcFilePath, @Nullable Surface surface,
                                 @Nullable DecodeListener listener) {
        boolean vsucceed = doDecode(srcFilePath, MEDIA_TYPE_VIDEO, surface, listener);
        boolean asucceed = doDecode(srcFilePath, MEDIA_TYPE_AUDIO, surface, listener);
        return vsucceed && asucceed;
    }

    private static boolean doDecode(String src, String dst, int mediaType) {
        FileOutputStream fos;
        try {
            fos = new FileOutputStream(dst);
        } catch (FileNotFoundException e) {
            e.printStackTrace();
            return false;
        }

        boolean succeed = doDecode(src, mediaType, null, new DecodeListener() {
            @Override
            public void onDecodedImage(byte[] data) {
                try {
                    fos.write(data);
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }

            @Override
            public void onDecodedSample(byte[] data) {
                try {
                    fos.write(data);
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        });

        try {
            fos.flush();
            fos.close();
        } catch (IOException e) {
            e.printStackTrace();
        }

        return succeed;
    }

    private static boolean doDecode(String src, int mediaType, Surface surface,
                                    DecodeListener listener) {
        MediaExtractor extractor = null;
        MediaCodec decoder = null;
        boolean decodeSucceed = false;
        boolean exceptionOccur = false;
        try {
            extractor = new MediaExtractor();
            extractor.setDataSource(src);
            decoder = doDecode(extractor, mediaType, surface, listener);
        } catch (IOException e) {
            Log.e(TAG, "doDecode exception: " + e);
            exceptionOccur = true;
        } finally {
            if (extractor != null) {
                extractor.release();
            }
            if (decoder != null) {
                decodeSucceed = true;
                decoder.stop();
                decoder.release();
            }
        }
        return !exceptionOccur && decodeSucceed;
    }

    private static MediaCodec doDecode(MediaExtractor extractor, int mediaType, Surface surface,
                                       DecodeListener listener) throws IOException {
        MediaFormat format = selectTrack(extractor, mediaType);
        if (format == null) {
            Log.e(TAG, "doDecode no " + mediaType + " track");
            return null;
        }
        Log.d(TAG, "docde format: " + format.toString());

        MediaCodec decoder = MediaCodec.createDecoderByType(format.getString(MediaFormat.KEY_MIME));
        decoder.configure(format, surface, null, 0);
        decoder.start();

        ByteBuffer[] inputBuffers = decoder.getInputBuffers();
        ByteBuffer[] outputBuffers = decoder.getOutputBuffers();
        MediaCodec.BufferInfo bufferInfo = new MediaCodec.BufferInfo();

        boolean inputEof = false;
        boolean outputEof = false;
        while (!outputEof) {
            if (!inputEof) {
                int inIndex = decoder.dequeueInputBuffer(DEFAULT_TIMEOUT);
                if (inIndex >= 0) {
                    ByteBuffer inputBuffer = inputBuffers[inIndex];
                    inputBuffer.clear();
                    int size = extractor.readSampleData(inputBuffer, 0);
                    if (size < 0) {
                        inputEof = true;
                        decoder.queueInputBuffer(inIndex, 0, 0,
                                0, MediaCodec.BUFFER_FLAG_END_OF_STREAM);
                    } else {
                        decoder.queueInputBuffer(inIndex, 0, size, extractor.getSampleTime(), 0);
                        extractor.advance();
                    }
                }
            }

            int outIndex = decoder.dequeueOutputBuffer(bufferInfo, DEFAULT_TIMEOUT);
            if (outIndex >= 0) {
                if ((bufferInfo.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) != 0) {
                    decoder.releaseOutputBuffer(outIndex, false);
                    continue;
                }

                if (bufferInfo.size != 0) {
                    ByteBuffer outputBuffer = outputBuffers[outIndex];
                    outputBuffer.position(bufferInfo.offset);
                    outputBuffer.limit(bufferInfo.offset + bufferInfo.size);
                    byte[] data = new byte[bufferInfo.size];
                    outputBuffer.get(data);
                    if (mediaType == MEDIA_TYPE_VIDEO && listener != null) {
                        listener.onDecodedImage(data);
                    } else if (listener != null) {
                        listener.onDecodedSample(data);
                    }
                }

                decoder.releaseOutputBuffer(outIndex, surface != null);

                if ((bufferInfo.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) {
                    outputEof = true;
                }
            } else if (outIndex == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED) {
                outputBuffers = decoder.getOutputBuffers();
                Log.d(TAG, "decoder output buffer have changed");
            } else if (outIndex == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
                MediaFormat tmp = decoder.getOutputFormat();
                Log.d(TAG, "decoder output format change to " + tmp.toString());
            }

        }

        return decoder;
    }

    private static MediaFormat selectTrack(MediaExtractor extractor, int mediaType) {
        MediaFormat format = null;
        int trackCount = extractor.getTrackCount();
        for (int i = 0; i < trackCount; ++i) {
            MediaFormat tmpFormat = extractor.getTrackFormat(i);
            if (getMediaType(tmpFormat) == mediaType) {
                format = tmpFormat;
                extractor.selectTrack(i);
                break;
            }
        }
        return format;
    }

    private static int getMediaType(MediaFormat format) {
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

    public interface DecodeListener {
        void onDecodedImage(byte[] data);

        void onDecodedSample(byte[] data);
    }


}
