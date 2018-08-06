package com.steven.avgraphics.activity;

import android.content.res.AssetManager;
import android.opengl.Matrix;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.Button;

import com.steven.avgraphics.BaseActivity;
import com.steven.avgraphics.R;
import com.steven.avgraphics.module.av.AVInfo;
import com.steven.avgraphics.module.av.HWCodec;
import com.steven.avgraphics.module.av.HWDecoder;
import com.steven.avgraphics.util.Utils;

import java.io.File;

public class VideoPlayActivity extends BaseActivity {

    private SurfaceView mSurfaceView;
    private Button mBtnStart;
    private Button mBtnStop;
    private Surface mSurface;

    private int mSurfaceWidth;
    private int mSurfaceHeight;
    private int mImageWidth;
    private int mImageHeight;
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
        setImageSize(Utils.getHWRecordOutput());
        mBtnStop.setEnabled(true);
        mBtnStart.setEnabled(false);
        _start(mSurface, mSurfaceWidth, mSurfaceHeight, mImageWidth, mImageHeight, getAssets());
        new Thread(() -> HWDecoder.decode(Utils.getHWRecordOutput(), null, new DecodeListener())).start();
    }

    private void setImageSize(String filePath) {
        mIsPlaying = true;
        AVInfo info = HWCodec.getAVInfo(filePath);
        mImageWidth = mSurfaceWidth;
        mImageHeight = mSurfaceHeight;
        if (info != null) {
            Log.i(TAG, "video start playing: \n" + info.toString());
            mImageWidth = info.width;
            mImageHeight = info.height;
        }
    }

    private void stop() {
        mIsPlaying = false;
        mBtnStart.setEnabled(true);
        mBtnStop.setEnabled(false);
        new Handler().postDelayed(VideoPlayActivity::_stop, 500);
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
            _stop();
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
    }

    private static native void _start(Surface surface, int width, int height, int imgWidth,
                                      int imgHeight, AssetManager manager);

    private static native void _draw(byte[] pixel, int length, int imgWidth, int imgHeight,
                                     float[] matrix);

    private static native void _stop();

}
