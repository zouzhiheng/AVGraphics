package com.steven.avgraphics.util;


import android.graphics.ImageFormat;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.media.MediaMuxer;
import android.os.Build;
import android.util.Log;
import android.util.SparseIntArray;

import com.steven.avgraphics.R;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

@SuppressWarnings("WeakerAccess")
public class HWCodec {

    private static final String TAG = "HWCodec";

    private static final long DEFAULT_TIMEOUT = 10 * 1000;

    private static final int MEDIA_TYPE_VIDEO = 1;
    private static final int MEDIA_TYPE_AUDIO = 2;
    private static final int MEDIA_TYPE_UNKNOWN = 0;

    private static final String MIME_TYPE_AVC = "video/avc";
    private static final String MIME_TYPE_AAC = "audio/mp4a-latm";

    public static boolean decode(String srcFilePath, String yuvDst, String pcmDst) {
        boolean vsucceed = decodeVideo(srcFilePath, yuvDst);
        boolean asucceed = decodeAudio(srcFilePath, pcmDst);
        return vsucceed && asucceed;
    }

    public static boolean decodeVideo(String srcFilePath, String dstFilePath) {
        return doDecode(srcFilePath, dstFilePath, MEDIA_TYPE_VIDEO);
    }

    public static boolean decodeAudio(String srcFilePath, String dstFilePath) {
        return doDecode(srcFilePath, dstFilePath, MEDIA_TYPE_AUDIO);
    }

    private static boolean doDecode(String src, String dst, int mediaType) {
        MediaExtractor extractor = null;
        MediaCodec decoder = null;
        FileOutputStream fos = null;
        boolean decodeSucceed = false;
        boolean exceptionOccur = false;
        try {
            extractor = new MediaExtractor();
            extractor.setDataSource(src);
            fos = new FileOutputStream(dst);
            decoder = doDecode(extractor, fos, mediaType);
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
            if (fos != null) {
                try {
                    fos.flush();
                    fos.close();
                } catch (IOException e) {
                    Log.e(TAG, "doDecode close fos error: " + e.getLocalizedMessage());
                }
            }
        }
        return !exceptionOccur && decodeSucceed;
    }

    private static MediaCodec doDecode(MediaExtractor extractor, FileOutputStream fos,
                                       int mediaType) throws IOException {
        MediaFormat format = selectTrack(extractor, mediaType);
        if (format == null) {
            Log.e(TAG, "doDecode no " + mediaType + " track");
            return null;
        }
        Log.d(TAG, "docde format: " + format.toString());

        MediaCodec decoder = MediaCodec.createDecoderByType(format.getString(MediaFormat.KEY_MIME));
        decoder.configure(format, null, null, 0);
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
                    fos.write(data);
                }

                decoder.releaseOutputBuffer(outIndex, false);

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

    public static class RecorderWrapper {

        private static final long MAX_TIMEOUT = 3000;

        private ExecutorService mVExecutor;
        private ExecutorService mAExecutor;
        private Recorder mRecorder = new Recorder();

        private int mImageFormat;

        public boolean init(int width, int height, int imageFormat, int bitRate, int sampleRate,
                            int channels, String dstFilePath) {
            mImageFormat = imageFormat;
            int colorFormat;
            if (imageFormat == ImageFormat.NV21) {
                colorFormat = MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar;
            } else if (imageFormat == ImageFormat.YV12) {
                colorFormat = MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Planar;
            } else {
                return false;
            }
            try {
                mRecorder.init(width, height, colorFormat, bitRate, sampleRate, channels, dstFilePath);
            } catch (Exception e) {
                e.printStackTrace();
                return false;
            }

            mVExecutor = Executors.newSingleThreadExecutor();
            mAExecutor = Executors.newSingleThreadExecutor();

            return true;
        }

        public void recordImage(byte[] image) {
            mVExecutor.execute(() -> {
                try {
                    convertToYuv420(image, mImageFormat);
                    mRecorder.recordImage(image);
                } catch (Exception e) {
                    e.printStackTrace();
                }
            });
        }

        // 交换 u、v 数据，以匹配 MediaCodec 的颜色格式
        private void convertToYuv420(byte[] data, int imageFormat) {
            if (imageFormat == ImageFormat.NV21) {
                nv21ToYuv420sp(data);
            } else if (imageFormat == ImageFormat.YV12) {
                yv12ToYuv420p(data);
            }
        }

        private void nv21ToYuv420sp(byte[] data) {
            int yLen = data.length * 2 / 3;
            for (int i = yLen; i < data.length - 1; i += 2) {
                byte tmp = data[i];
                data[i] = data[i + 1];
                data[i + 1] = tmp;
            }
        }

        private void yv12ToYuv420p(byte[] data) {
            int yLen = data.length * 2 / 3;
            int vLen = yLen / 2;
            for (int i = yLen; i < yLen + vLen; i++) {
                byte tmp = data[i];
                data[i] = data[i + vLen];
                data[i + vLen] = tmp;
            }
        }

        public void recordSample(byte[] sample) {
            mAExecutor.execute(() -> {
                try {
                    mRecorder.recordSample(sample);
                } catch (Exception e) {
                    e.printStackTrace();
                }
            });
        }

        public void stop() {
            Executors.newSingleThreadExecutor().execute(() -> {
                try {
                    mVExecutor.shutdown();
                    mVExecutor.awaitTermination(MAX_TIMEOUT, TimeUnit.MILLISECONDS);
                    mAExecutor.shutdown();
                    mAExecutor.awaitTermination(MAX_TIMEOUT, TimeUnit.MICROSECONDS);
                    mRecorder.stop();
                    ToastHelper.showOnUiThread(R.string.hwcodec_msg_record_complete);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            });
        }

    }

    public static class Recorder {
        private static final int DEFAULT_FRAME_RATE = 30;
        private static final int DEFAULT_IFRAME_INTERVAL = 5;
        private static final int DEFAULT_BITRATE_AUDIO = 128 * 1000;

        private MediaMuxer mMuxer;
        private MediaCodec mVideoEncoder;
        private MediaCodec mAudioEncoder;
        private MediaCodec.BufferInfo mVBufferInfo;
        private MediaCodec.BufferInfo mABufferInfo;

        private boolean mIsInitialized = false;
        private long mVStartTime;
        private long mAStartTime;
        private int mVTrackIndex;
        private int mATrackIndex;
        private volatile boolean mMuxerStarted;

        public void init(int width, int height, int colorFormat, int bitRate, int sampleRate,
                         int channels, String dstFilePath) throws Exception {

            if (getCodecInfo(MIME_TYPE_AVC) == null || getCodecInfo(MIME_TYPE_AAC) == null) {
                throw new Exception("cannot find suitable codec");
            }

            MediaFormat videoFormat = MediaFormat.createVideoFormat(MIME_TYPE_AVC, width, height);
            videoFormat.setInteger(MediaFormat.KEY_BIT_RATE, bitRate);
            videoFormat.setInteger(MediaFormat.KEY_FRAME_RATE, DEFAULT_FRAME_RATE);
            videoFormat.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, DEFAULT_IFRAME_INTERVAL);
            videoFormat.setInteger(MediaFormat.KEY_COLOR_FORMAT, colorFormat);

            mVideoEncoder = MediaCodec.createEncoderByType(MIME_TYPE_AVC);
            mVideoEncoder.configure(videoFormat, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
            mVideoEncoder.start();

            MediaFormat audioFormat = MediaFormat.createAudioFormat(MIME_TYPE_AAC, sampleRate, channels);
            audioFormat.setInteger(MediaFormat.KEY_AAC_PROFILE, MediaCodecInfo.CodecProfileLevel.AACObjectLC);
            audioFormat.setInteger(MediaFormat.KEY_BIT_RATE, DEFAULT_BITRATE_AUDIO);

            mAudioEncoder = MediaCodec.createEncoderByType(MIME_TYPE_AAC);
            mAudioEncoder.configure(audioFormat, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
            mAudioEncoder.start();

            File file = new File(dstFilePath);
            if (file.exists() && !file.delete()) {
                Log.w(TAG, "delete file failed");
            }

            mMuxer = new MediaMuxer(dstFilePath, MediaMuxer.OutputFormat.MUXER_OUTPUT_MPEG_4);
            mMuxerStarted = false;
            mVTrackIndex = -1;
            mATrackIndex = -1;
            mVStartTime = -1;
            mAStartTime = -1;

            mVBufferInfo = new MediaCodec.BufferInfo();
            mABufferInfo = new MediaCodec.BufferInfo();
            mIsInitialized = true;
            Log.i(TAG, "Recorder initialized");
        }

        private static MediaCodecInfo getCodecInfo(final String mimeType) {
            final int numCodecs = MediaCodecList.getCodecCount();
            for (int i = 0; i < numCodecs; i++) {
                final MediaCodecInfo codecInfo = MediaCodecList.getCodecInfoAt(i);
                if (!codecInfo.isEncoder()) {
                    continue;
                }
                final String[] types = codecInfo.getSupportedTypes();
                for (String type : types) {
                    if (type.equalsIgnoreCase(mimeType)) {
                        return codecInfo;
                    }
                }
            }
            return null;
        }

        public void recordImage(byte[] image) throws Exception {
            long pts;
            if (mVStartTime == -1) {
                mVStartTime = System.nanoTime();
                pts = 0;
            } else {
                pts = (System.nanoTime() - mVStartTime) / 1000;
            }
            doRecord(mVideoEncoder, mVBufferInfo, image, pts);
        }

        public void recordSample(byte[] sample) throws Exception {
            long pts;
            if (mAStartTime == -1) {
                mAStartTime = System.nanoTime();
                pts = 0;
            } else {
                pts = (System.nanoTime() - mAStartTime) / 1000;
            }
            doRecord(mAudioEncoder, mABufferInfo, sample, pts);
        }

        private void doRecord(MediaCodec encoder, MediaCodec.BufferInfo bufferInfo, byte[] data,
                              long pts) throws Exception {
            if (!mIsInitialized) {
                Log.e(TAG, "Recorder must be initialized!");
                return;
            }
            int index = encoder.dequeueInputBuffer(DEFAULT_TIMEOUT);
            ByteBuffer[] inputBuffers = encoder.getInputBuffers();
            ByteBuffer buffer = inputBuffers[index];
            if (index >= 0) {
                buffer.clear();
                buffer.put(data);
                encoder.queueInputBuffer(index, 0, data.length, pts, 0);
            }
            drainEncoder(encoder, bufferInfo);
        }

        private void drainEncoder(MediaCodec encoder, MediaCodec.BufferInfo bufferInfo) throws Exception {
            int trackIndex = encoder == mVideoEncoder ? mVTrackIndex : mATrackIndex;
            ByteBuffer[] outputBuffers = encoder.getOutputBuffers();
            while (true) {
                int index = encoder.dequeueOutputBuffer(bufferInfo, DEFAULT_TIMEOUT);
                if (index == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED) {
                    outputBuffers = encoder.getOutputBuffers();
                } else if (index == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
                    trackIndex = addTrackIndex(encoder);
                } else if (index == MediaCodec.INFO_TRY_AGAIN_LATER) {
                    break;
                } else if (index < 0) {
                    Log.w(TAG, "drainEncoder unexpected result: " + index);
                } else {
                    if ((bufferInfo.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) != 0) {
                        continue;
                    }

                    if (bufferInfo.size != 0) {
                        ByteBuffer outputBuffer = outputBuffers[index];

                        if (outputBuffer == null) {
                            throw new RuntimeException("drainEncoder get outputBuffer " + index + " was null");
                        }

                        synchronized (this) {
                            if (!mMuxerStarted) {
                                wait();
                            }
                        }

                        outputBuffer.position(bufferInfo.offset);
                        outputBuffer.limit(bufferInfo.offset + bufferInfo.size);
                        mMuxer.writeSampleData(trackIndex, outputBuffer, bufferInfo);
                    }

                    encoder.releaseOutputBuffer(index, false);
                }
            }
        }

        private int addTrackIndex(MediaCodec encoder) {
            int trackIndex;
            synchronized (this) {
                MediaFormat format = encoder.getOutputFormat();
                if (getMediaType(format) == MEDIA_TYPE_VIDEO) {
                    mVTrackIndex = mMuxer.addTrack(format);
                    trackIndex = mVTrackIndex;
                } else {
                    mATrackIndex = mMuxer.addTrack(format);
                    trackIndex = mATrackIndex;
                }

                if (mVTrackIndex != -1 && mATrackIndex != -1) {
                    mMuxer.start();
                    mMuxerStarted = true;
                    notifyAll();
                    Log.i(TAG, "MediaMuxer has added all track, notifyAll");
                }
            }
            return trackIndex;
        }

        public void stop() {
            try {
                release();
            } catch (Exception e) {
                Log.e(TAG, "stop exception occur: " + e.getLocalizedMessage());
            }
            if (mIsInitialized) {
                Log.i(TAG, "Recorder released");
            }
            mIsInitialized = false;
        }
        
        private void release() throws Exception {
            if (mVideoEncoder != null) {
                mVideoEncoder.stop();
                mVideoEncoder.release();
                mVideoEncoder = null;
            }

            if (mAudioEncoder != null) {
                mAudioEncoder.stop();
                mAudioEncoder.release();
                mAudioEncoder = null;
            }

            if (mMuxer != null) {
                mMuxer.stop();
                mMuxer.release();
                mMuxer = null;
            }
        }

    }


}
