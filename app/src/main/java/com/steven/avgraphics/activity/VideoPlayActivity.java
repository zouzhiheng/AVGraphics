package com.steven.avgraphics.activity;

import android.content.res.AssetManager;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.media.MediaCodecInfo;
import android.opengl.Matrix;
import android.os.Bundle;
import android.os.CountDownTimer;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.Button;
import android.widget.TextView;

import com.steven.avgraphics.BaseActivity;
import com.steven.avgraphics.R;
import com.steven.avgraphics.module.av.AVInfo;
import com.steven.avgraphics.module.av.FFCodec;
import com.steven.avgraphics.module.av.Format;
import com.steven.avgraphics.module.av.HWDecoder;
import com.steven.avgraphics.module.av.OnDecodeListener;
import com.steven.avgraphics.util.ToastHelper;
import com.steven.avgraphics.util.Utils;

import java.io.File;

public class VideoPlayActivity extends BaseActivity {

    private static final int DEFAULT_FRAME_RATE = 24;
    private static final int DEFAULT_PIXEL_FORMAT = Format.PIXEL_FORMAT_NV12;
    private static final int DEFAULT_SAMPLE_RATE = 48000;
    private static final int DEFAULT_SAMPLE_FORMAT = Format.SAMPLE_FORMAT_16BIT;

    private SurfaceView mSurfaceView;
    private Button mBtnStart;
    private Button mBtnStop;
    private TextView mTvAVInfo;
    private TextView mTvTime;

    private Surface mSurface;
    private HWDecoder mDecoder = new HWDecoder();
    private DecodeListener mDecodeListener;
    private AudioTrack mAudioTrack;
    private AVInfo mAVInfo;
    private CountDownTimer mCountDownTimer;
    private File mFile;

    private int mSurfaceWidth;
    private int mSurfaceHeight;
    private int mImageWidth;
    private int mImageHeight;
    private int mFrameRate = DEFAULT_FRAME_RATE;
    private int mPixelFormat = DEFAULT_PIXEL_FORMAT;
    private int mSampleRate;
    private int mSampleFormat;
    private int mChannels;
    private float[] mMatrix = new float[16];
    private boolean mIsPlaying = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_video_play);
        init();
    }

    private void init() {
        Matrix.setIdentityM(mMatrix, 0);
        mFile = new File(Utils.getFFRecordOutput());
        mAVInfo = FFCodec.getAVInfo(mFile.getAbsolutePath());
        findView();
        showAVInfo();
        layoutSurfaceView();
        setListener();
    }

    private void findView() {
        mSurfaceView = findViewById(R.id.vplay_sv_window);
        mBtnStart = findViewById(R.id.vplay_btn_start);
        mBtnStop = findViewById(R.id.vplay_btn_stop);
        mTvAVInfo = findViewById(R.id.vplay_tv_avinfo);
        mTvTime = findViewById(R.id.vplay_tv_time);
    }

    private void showAVInfo() {
        if (mAVInfo != null) {
            String str = "file: " + mFile.getAbsolutePath() + "\n"
                    + "width: " + mAVInfo.width + ", height: " + mAVInfo.height + "\n"
                    + "frame rate: " + mAVInfo.frameRate + ", pixel format: "
                    + Format.getPixelFormatName(mAVInfo.pixelFommat) + "\n"
                    + "sample rate: " + mAVInfo.sampleRate + ", channels: " + mAVInfo.channels
                    + ", sample format: " + Format.getSampleFormatName(mAVInfo.sampleFormat) + "\n"
                    + "duration: " + mAVInfo.vDuration / 1000 / 1000 + "s";
            mTvAVInfo.setText(str);
        }
    }

    private void layoutSurfaceView() {
        double ratio = mAVInfo == null ? 1 : 1.0 * mAVInfo.height / mAVInfo.width;
        int width = Utils.getScreenWidth();
        int heigth = Utils.getScreenWidth();
        if (ratio > 1) {
            width = (int) (heigth / ratio);
        } else {
            heigth = (int) (width * ratio);
        }
        mSurfaceView.getLayoutParams().width = width;
        mSurfaceView.getLayoutParams().height = heigth;
    }

    private void setListener() {
        mSurfaceView.getHolder().addCallback(new SurfaceCallback());
        mBtnStart.setOnClickListener(v -> start());
        mBtnStop.setOnClickListener(v -> stop());
    }

    private void start() {
        if (!mFile.exists()) {
            Log.e(TAG, "start video player failed: file not found");
            ToastHelper.show(R.string.vplay_msg_no_file);
            return;
        }

        mIsPlaying = true;
        mBtnStop.setEnabled(true);
        mBtnStart.setEnabled(false);

        setupVideoParams();
        startDecode();
        setupAudioTrack();
//        _startSL(mSampleRate, DEFAULT_SAMPLE_FORMAT, mChannels);
        _startGL(mSurface, mSurfaceWidth, mSurfaceHeight, mImageWidth, mImageHeight, mFrameRate,
                getAssets());
        startCounDownTimer();
    }

    private void setupVideoParams() {
        mImageWidth = mAVInfo != null && mAVInfo.width > 0 ? mAVInfo.width : mSurfaceWidth;
        mImageHeight = mAVInfo != null && mAVInfo.height > 0 ? mAVInfo.height : mSurfaceHeight;
        mFrameRate = mAVInfo != null && mAVInfo.frameRate > 0 ? mAVInfo.frameRate : DEFAULT_FRAME_RATE;
        mChannels = mAVInfo != null && mAVInfo.channels == 2 ? 2 : 1;
        mSampleRate = mAVInfo != null && mAVInfo.sampleRate > 0 ? mAVInfo.sampleRate : DEFAULT_SAMPLE_RATE;
        // MediaCodec 解码出来的基本都是 NV12
        mPixelFormat = DEFAULT_PIXEL_FORMAT;
        mSampleFormat = mAVInfo != null && mAVInfo.sampleFormat > 0 ? mAVInfo.sampleFormat : DEFAULT_SAMPLE_FORMAT;
    }

    private void setupAudioTrack() {
        int channelConfig = mChannels == 1 ? AudioFormat.CHANNEL_OUT_MONO : AudioFormat.CHANNEL_OUT_STEREO;
        int bufferSize = AudioTrack.getMinBufferSize(mSampleRate, channelConfig,
                Format.getAudioFormat(mSampleFormat));
        mAudioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, mSampleRate, channelConfig,
                Format.getAudioFormat(mSampleFormat), bufferSize, AudioTrack.MODE_STREAM);
    }

    private void startDecode() {
        mDecodeListener = new DecodeListener();
        mDecoder.setDecodeWithPts(true);
        mDecoder.start(mFile.getAbsolutePath(), mDecodeListener);
    }

    private void startCounDownTimer() {
        mCountDownTimer = new CountDownTimer(mAVInfo.vDuration + 1000, 1000) {

            long mPass = 0;

            @Override
            public void onTick(long millisUntilFinished) {
                String str = mPass + "s";
                mTvTime.setText(str);
                mPass++;
            }

            @Override
            public void onFinish() {

            }
        };
        mCountDownTimer.start();
    }

    private void stop() {
        mIsPlaying = false;
        mBtnStart.setEnabled(true);
        mBtnStop.setEnabled(false);
        mDecoder.stop();
        if (mCountDownTimer != null) {
            mCountDownTimer.cancel();
        }
    }

    private synchronized void releaseAudioTrack() {
        if (mAudioTrack != null) {
            mAudioTrack.stop();
            mAudioTrack.release();
            mAudioTrack = null;
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        mDecodeListener = null;
        stop();
        releaseAudioTrack();
//        _stopSL();
        _stopGL();
    }

    private class SurfaceCallback implements SurfaceHolder.Callback {

        @Override
        public void surfaceCreated(SurfaceHolder holder) {
            mSurface = holder.getSurface();
        }

        @Override
        public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
            mSurfaceWidth = width;
            mSurfaceHeight = height;
        }

        @Override
        public void surfaceDestroyed(SurfaceHolder holder) {

        }

    }

    private class DecodeListener implements OnDecodeListener {

        @Override
        public void onImageDecoded(byte[] data) {
            if (mIsPlaying) {
                _drawGL(data, data.length, mImageWidth, mImageHeight, mPixelFormat, mMatrix);
            }
        }

        @Override
        public void onSampleDecoded(byte[] data) {
            synchronized (VideoPlayActivity.this) {
                if (mIsPlaying) {
                    mAudioTrack.write(data, 0, data.length);
                    mAudioTrack.play();
                }
            }
//            _writeSL(data, data.length);
        }

        @Override
        public void onDecodeEnded(boolean vsucceed, boolean asucceed) {
            Utils.runOnUiThread(() -> {
                stop();
                releaseAudioTrack();
                _stopGL();
//                _stopSL();
            });
        }
    }

    private static native void _startGL(Surface surface, int width, int height, int imgWidth,
                                        int imgHeight, int frameRate, AssetManager manager);

    private static native void _drawGL(byte[] pixel, int length, int imgWidth, int imgHeight,
                                       int pixelFormat, float[] matrix);

    private static native void _stopGL();

    // 如果要使用 OpenSL，可以取消下面的注释
//    private static native void _startSL(int sampleRate, int samleFormat, int channels);
//
//    private static native void _writeSL(byte[] data, int length);
//
//    private static native void _stopSL();

}
