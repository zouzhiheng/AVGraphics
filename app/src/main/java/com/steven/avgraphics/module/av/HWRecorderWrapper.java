package com.steven.avgraphics.module.av;


import android.graphics.ImageFormat;
import android.media.MediaCodecInfo;

import com.steven.avgraphics.R;
import com.steven.avgraphics.util.ToastHelper;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

public class HWRecorderWrapper {

    private static final long MAX_TIMEOUT = 3000;

    private ExecutorService mVExecutor;
    private ExecutorService mAExecutor;
    private HWRecorder mRecorder = new HWRecorder();

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
