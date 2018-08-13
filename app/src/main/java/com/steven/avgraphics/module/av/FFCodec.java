package com.steven.avgraphics.module.av;


import android.os.Handler;
import android.os.Looper;
import android.support.annotation.NonNull;

@SuppressWarnings("ALL")
public class FFCodec {

    public static final int PIXEL_FORMAT_NONE = 0;
    public static final int PIXEL_FORMAT_NV21 = 1;
    public static final int PIXEL_FORMAT_YV12 = 2;

    public static final int SAMPLE_FORMAT_NONE = 0;
    public static final int SAMPLE_FORMAT_8BIT = 1;
    public static final int SAMPLE_FORMAT_16BIT = 2;

    private Handler mHandler = new Handler(Looper.getMainLooper());
    private RecordListener mRecordListener;

    public static native void _decode(String srcFile, String yuvDst, String pcmDst);

    public static boolean initRecorder(RecordParams params, RecordListener listener) {
        return new FFCodec().doInitRecorder(params, listener);
    }

    private boolean doInitRecorder(RecordParams params, RecordListener recordListener) {
        mRecordListener = recordListener;
        FilterParams filterParams = params.mFilterParams;
        String videoFilter = filterParams.mVideoFilter == null ? "null" : filterParams.mVideoFilter;
        String audioFilter = filterParams.mAudioFilter == null ? "anull" : filterParams.mAudioFilter;
        int result = _initRecorder(params.mDstFile, params.mWidth, params.mHeight, params.mPixelFormat,
                params.mFrameRate, params.mSampleFormat, params.mSampleRate, params.mChannels,
                params.mMaxBitRate, params.mQuality, filterParams.mCropX, filterParams.mCropY,
                filterParams.mCropW, filterParams.mCropH, filterParams.mRotateDegree,
                filterParams.mScaleW, filterParams.mScaleH, filterParams.mIsMirror,
                videoFilter, audioFilter);
        return result >= 0;
    }

    public static boolean recordImage(byte[] data, int length, int width, int height,
                                      int pixelFormat) {
        return _recordImage(data, length, width, height, pixelFormat) >= 0;
    }

    public static boolean recordSample(byte[] data, int length) {
        return _recordSamples(data, length) >= 0;
    }

    public static void stopRecord() {
        _stopRecord();
    }

    private native int _initRecorder(String dstFilePath, int width, int height, int pixelFormat,
                                     int frameRate, int sampleFormat, int sampleRate,
                                     int channels, long maxBitRate, int quality,
                                     int cropX, int cropY, int cropW, int cropH,
                                     int rotateDegree, int scaleW, int scaleH, boolean mirror,
                                     String videoFilter, String audioFilter);

    private static native int _recordImage(byte[] data, int length, int width, int height,
                                           int pixelFormat);

    private static native int _recordSamples(byte[] data, int length);

    private static native void _stopRecord();

    private void getRecordResultFromNative(final boolean succeed) {
        mHandler.post(() -> mRecordListener.onRecordCompleted(succeed));
    }

    public interface RecordListener {
        void onRecordCompleted(boolean succeed);
    }

    public static class RecordParams {
        private String mDstFile;
        private int mWidth;
        private int mHeight;
        private int mFrameRate;
        private int mSampleRate;
        private int mPixelFormat;
        // 16BIT 格式兼容性更好
        private int mSampleFormat;
        // 单声道效率更高
        private int mChannels;
        // 当设置的 maxBitRate 无法达到要求的 quality 时，quality 值会自动调整
        private long mMaxBitRate;
        // 仅推荐 17 ~ 28 之间，默认为 23，值越小，视频质量越高
        private int mQuality = 23;
        private FilterParams mFilterParams = new FilterParams();

        public RecordParams(@NonNull String dstFile, int width, int height, int frameRate,
                            int sampleRate, int pixelFormat, int sampleFormat, int channels) {
            mDstFile = dstFile;
            mWidth = width;
            mHeight = height;
            mFrameRate = frameRate;
            mSampleRate = sampleRate;
            mPixelFormat = pixelFormat;
            mSampleFormat = sampleFormat;
            mChannels = channels;
        }

        public void setMaxBitRate(int maxBitRate) {
            mMaxBitRate = maxBitRate;
        }

        public void setQuality(int quality) {
            mQuality = quality;
        }

        public void setFilterParams(FilterParams filterParams) {
            mFilterParams = filterParams;
        }
    }

    public static class FilterParams {
        // 处理顺序: 1) crop 2) rotate 3) scale 4) filter
        private int mCropX;
        private int mCropY;
        private int mCropW;
        private int mCropH;
        private int mRotateDegree;
        private int mScaleW;
        private int mScaleH;
        private boolean mIsMirror;
        private String mVideoFilter;
        private String mAudioFilter;

        public void setCrop(int x, int y, int w, int h) {
            mCropX = x;
            mCropY = y;
            mCropW = w;
            mCropH = h;
        }

        public void setRotate(int degree) {
            mRotateDegree = degree;
        }

        public void setScale(int targetWidth, int targetHeight) {
            mScaleW = targetWidth;
            mScaleH = targetHeight;
        }

        public void setMirror(boolean mirror) {
            mIsMirror = mirror;
        }

        public void setVideoFilter(String videoFilter) {
            mVideoFilter = videoFilter;
        }

        public void setAudioFilter(String audioFilter) {
            mAudioFilter = audioFilter;
        }
    }

}
