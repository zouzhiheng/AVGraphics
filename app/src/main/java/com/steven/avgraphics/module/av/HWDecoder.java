package com.steven.avgraphics.module.av;


import android.media.MediaCodec;
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.util.Log;
import android.view.Surface;

import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

public class HWDecoder {

    private static final String TAG = "HWDecoder";

    private static final long DEFAULT_TIMEOUT = 10 * 1000;

    private ExecutorService mExecutor = Executors.newCachedThreadPool();
    private boolean mIsDecoding = false;
    private boolean mIsEndDecodeVideo = false;
    private boolean mIsEndDecodeAudio = false;
    private boolean mIsDecodeVideoSucceed = false;
    private boolean mIsDecodeAudioSucceed = false;

    private boolean mIsDecodeWithPts = false;

    @SuppressWarnings("SameParameterValue")
    public void setDecodeWithPts(boolean decodeWithPts) {
        mIsDecodeWithPts = decodeWithPts;
    }

    public void start(String srcFilePath, String yuvDst, String pcmDst) {
        start(srcFilePath, yuvDst, pcmDst, null);
    }

    public void start(String srcFilePath, String yuvDst, String pcmDst, OnDecodeEndListener listener) {
        if (mIsDecoding) {
            Log.e(TAG, "start failed, decoder is working now");
            return;
        }
        mIsDecoding = true;
        resetValues();
        mExecutor.execute(() -> {
            mIsDecodeVideoSucceed = doDecode(srcFilePath, yuvDst, HWCodec.MEDIA_TYPE_VIDEO);
            mIsEndDecodeVideo = true;
            if (mIsEndDecodeAudio) {
                mIsDecoding = false;
                if (listener != null) {
                    listener.onDecodeEnded(mIsDecodeVideoSucceed, mIsDecodeAudioSucceed);
                }
            }
        });
        mExecutor.execute(() -> {
            mIsDecodeAudioSucceed = doDecode(srcFilePath, pcmDst, HWCodec.MEDIA_TYPE_AUDIO);
            mIsEndDecodeAudio = true;
            if (mIsEndDecodeVideo) {
                mIsDecoding = false;
                if (listener != null) {
                    listener.onDecodeEnded(mIsDecodeVideoSucceed, mIsDecodeAudioSucceed);
                }
            }
        });
    }

    public void start(@NonNull String srcFilePath, @Nullable Surface surface) {
        start(srcFilePath, surface, (OnDecodeListener) null);
    }

    public void start(@NonNull String srcFilePath, @Nullable Surface surface, @Nullable OnDecodeEndListener listener) {
        start(srcFilePath, surface, (OnDecodeListener) listener);
    }

    public void start(@NonNull String srcFilePath, @NonNull OnDecodeListener listener) {
        start(srcFilePath, null, listener);
    }

    private void start(@NonNull String srcFilePath, @Nullable Surface surface, @Nullable OnDecodeListener listener) {
        if (mIsDecoding) {
            Log.e(TAG, "start failed, decoder is working now");
            return;
        }
        mIsDecoding = true;
        resetValues();
        mExecutor.execute(() -> {
            mIsDecodeVideoSucceed = doDecode(srcFilePath, HWCodec.MEDIA_TYPE_VIDEO, surface, listener);
            mIsEndDecodeVideo = true;
            if (mIsEndDecodeAudio) {
                mIsDecoding = false;
                if (listener != null) {
                    listener.onDecodeEnded(mIsDecodeVideoSucceed, mIsDecodeVideoSucceed);
                }
            }
        });
        mExecutor.execute(() -> {
            mIsDecodeAudioSucceed = doDecode(srcFilePath, HWCodec.MEDIA_TYPE_AUDIO, null, listener);
            mIsEndDecodeAudio = true;
            if (mIsEndDecodeVideo) {
                mIsDecoding = false;
                if (listener != null) {
                    listener.onDecodeEnded(mIsDecodeVideoSucceed, mIsDecodeAudioSucceed);
                }
            }
        });
    }

    private void resetValues() {
        mIsEndDecodeVideo = false;
        mIsEndDecodeAudio = false;
        mIsDecodeVideoSucceed = false;
        mIsDecodeAudioSucceed = false;
    }

    public void stop() {
        mIsDecoding = false;
    }

    private boolean doDecode(String src, String dst, int mediaType) {
        FileOutputStream fos;
        try {
            fos = new FileOutputStream(dst);
        } catch (FileNotFoundException e) {
            e.printStackTrace();
            return false;
        }

        return doDecode(src, mediaType, null, new OnDecodeListener() {
            @Override
            public void onImageDecoded(byte[] data) {
                try {
                    fos.write(data);
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }

            @Override
            public void onSampleDecoded(byte[] data) {
                try {
                    fos.write(data);
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }

            @Override
            public void onDecodeEnded(boolean vsucceed, boolean asucceed) {
                try {
                    fos.flush();
                    fos.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        });
    }

    private boolean doDecode(String src, int mediaType, Surface surface,
                             OnDecodeListener listener) {
        MediaExtractor extractor = null;
        MediaCodec decoder = null;
        boolean decodeSucceed = false;
        boolean exceptionOccur = false;
        try {
            extractor = new MediaExtractor();
            extractor.setDataSource(src);
            decoder = doDecode(extractor, mediaType, surface, listener);
        } catch (IOException | InterruptedException e) {
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

    private MediaCodec doDecode(MediaExtractor extractor, int mediaType, Surface surface,
                                OnDecodeListener listener) throws IOException, InterruptedException {
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

        long passTime;
        long startTime = 0;

        boolean inputEof = false;
        boolean outputEof = false;
        while (!outputEof) {
            if (!mIsDecoding) {
                Log.i(TAG, (mediaType == HWCodec.MEDIA_TYPE_VIDEO ? "video" : "audio") + " decoder stopped");
                break;
            }

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

                    if (mIsDecodeWithPts) {
                        if (startTime == 0) {
                            startTime = System.nanoTime();
                        } else {
                            passTime = (System.nanoTime() - startTime) / 1000;
                            if (passTime < bufferInfo.presentationTimeUs) {
                                TimeUnit.MICROSECONDS.sleep(bufferInfo.presentationTimeUs - passTime);
                            }
                        }
                    }

                    if (mediaType == HWCodec.MEDIA_TYPE_VIDEO && listener != null) {
                        listener.onImageDecoded(data);
                    } else if (listener != null) {
                        listener.onSampleDecoded(data);
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
            if (HWCodec.getMediaType(tmpFormat) == mediaType) {
                format = tmpFormat;
                extractor.selectTrack(i);
                break;
            }
        }
        return format;
    }

    public interface OnDecodeEndListener extends OnDecodeListener {

        @Override
        default void onImageDecoded(byte[] data) {

        }

        @Override
        default void onSampleDecoded(byte[] data) {

        }

        @Override
        void onDecodeEnded(boolean vsucceed, boolean asucceed);
    }

}
