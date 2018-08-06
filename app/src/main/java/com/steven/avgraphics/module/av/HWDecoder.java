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

public class HWDecoder {

    private static final String TAG = "HWDecoder";

    private static final long DEFAULT_TIMEOUT = 10 * 1000;

    public static boolean decode(String srcFilePath, String yuvDst, String pcmDst) {
        boolean vsucceed = doDecode(srcFilePath, yuvDst, HWCodec.MEDIA_TYPE_VIDEO);
        Log.i(TAG, "decode video: " + vsucceed);
        boolean asucceed = doDecode(srcFilePath, pcmDst, HWCodec.MEDIA_TYPE_AUDIO);
        Log.i(TAG, "decode audio: " + asucceed);
        return vsucceed && asucceed;
    }

    public static boolean decode(@NonNull String srcFilePath, @Nullable Surface surface,
                                 @Nullable OnDecodeListener listener) {
        boolean vsucceed = doDecode(srcFilePath, HWCodec.MEDIA_TYPE_VIDEO, surface, listener);
        Log.i(TAG, "decode video: " + vsucceed);
        boolean asucceed = doDecode(srcFilePath, HWCodec.MEDIA_TYPE_AUDIO, null, listener);
        Log.i(TAG, "decode audio: " + asucceed);
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

        boolean succeed = doDecode(src, mediaType, null, new OnDecodeListener() {
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
                                    OnDecodeListener listener) {
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
                                       OnDecodeListener listener) throws IOException {
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

    public interface OnDecodeListener {
        void onImageDecoded(byte[] data);

        void onSampleDecoded(byte[] data);
    }

}
