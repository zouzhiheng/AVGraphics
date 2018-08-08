package com.steven.avgraphics.activity;

import android.content.res.AssetManager;
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
import com.steven.avgraphics.module.av.HWCodec;
import com.steven.avgraphics.module.av.HWDecoder;
import com.steven.avgraphics.util.Utils;

import java.io.File;

public class VideoPlayActivity extends BaseActivity {

    private static final int DEFAULT_FRAME_RATE = 24;

    private SurfaceView mSurfaceView;
    private Button mBtnStart;
    private Button mBtnStop;
    private TextView mTvAVInfo;
    private TextView mTvTime;

    private Surface mSurface;
    private HWDecoder mDecoder = new HWDecoder();
    private DecodeListener mDecodeListener;
    private AVInfo mAVInfo;
    private CountDownTimer mCountDownTimer;

    private int mSurfaceWidth;
    private int mSurfaceHeight;
    private int mImageWidth;
    private int mImageHeight;
    private int mFrameRate = DEFAULT_FRAME_RATE;
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
        findView();
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

    private void layoutSurfaceView() {
        AVInfo info = HWCodec.getAVInfo(Utils.getHWRecordOutput());
        mSurfaceView.getLayoutParams().width = Utils.getScreenWidth();
        mSurfaceView.getLayoutParams().height = info == null ? Utils.getScreenWidth()
                : Utils.getScreenWidth() * info.height / info.width;
    }

    private void setListener() {
        mSurfaceView.getHolder().addCallback(new SurfaceCallback());
        mBtnStart.setOnClickListener(v -> start());
        mBtnStop.setOnClickListener(v -> stop());
    }

    private void start() {
        File file = new File(Utils.getHWRecordOutput());
        if (!file.exists()) {
            Log.e(TAG, "start video player failed: file not found");
            return;
        }
        mIsPlaying = true;
        mDecodeListener = new DecodeListener();
        mAVInfo = HWCodec.getAVInfo(Utils.getHWRecordOutput());
        assert mAVInfo != null;
        setVideoParams();
        showAVInfo();
        mBtnStop.setEnabled(true);
        mBtnStart.setEnabled(false);
        mDecoder.setFrameRate(mFrameRate);
        mDecoder.start(Utils.getHWRecordOutput(), mDecodeListener);
        _start(mSurface, mSurfaceWidth, mSurfaceHeight, mImageWidth, mImageHeight, mFrameRate,
                getAssets());
        startCounDownTimer();
    }

    private void setVideoParams() {
        mImageWidth = mAVInfo != null && mAVInfo.width > 0 ? mAVInfo.width : mSurfaceWidth;
        mImageHeight = mAVInfo != null && mAVInfo.height > 0 ? mAVInfo.height : mSurfaceHeight;
        mFrameRate = mAVInfo != null && mAVInfo.frameRate > 0 ? mAVInfo.frameRate : DEFAULT_FRAME_RATE;
        Log.i(TAG, "frame rate: " + mFrameRate);
    }

    private void showAVInfo() {
        if (mAVInfo != null) {
            String str = "width: " + mAVInfo.width + ", height: " + mAVInfo.height + "\nframe rate: "
                    + mAVInfo.frameRate + ", duration: " + mAVInfo.vDuration / 1000 / 1000 + "s";
            mTvAVInfo.setText(str);
        }
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
        mCountDownTimer.cancel();
    }

    @Override
    protected void onPause() {
        super.onPause();
        mIsPlaying = false;
        mDecodeListener = null;
        mDecoder.stop();
        _stop();
        if (mCountDownTimer != null) {
            mCountDownTimer.cancel();
        }
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

    private class DecodeListener implements HWDecoder.OnDecodeListener {

        @Override
        public void onImageDecoded(byte[] data) {
            if (mIsPlaying) {
                _draw(data, data.length, mImageWidth, mImageHeight, mMatrix);
            }
        }

        @Override
        public void onSampleDecoded(byte[] data) {

        }

        @Override
        public void onDecodeEnded(boolean vsucceed, boolean asucceed) {
            mIsPlaying = false;
            _stop();
            Utils.runOnUiThread(() -> {
                mBtnStart.setEnabled(true);
                mBtnStop.setEnabled(false);
                mCountDownTimer.cancel();
            });
        }
    }

    private static native void _start(Surface surface, int width, int height, int imgWidth,
                                      int imgHeight, int frameRate, AssetManager manager);

    private static native void _draw(byte[] pixel, int length, int imgWidth, int imgHeight,
                                     float[] matrix);

    private static native void _stop();

}
