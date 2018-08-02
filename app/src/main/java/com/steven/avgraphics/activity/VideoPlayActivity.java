package com.steven.avgraphics.activity;

import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.Button;

import com.steven.avgraphics.R;
import com.steven.avgraphics.module.av.AVInfo;
import com.steven.avgraphics.module.av.HWCodec;
import com.steven.avgraphics.util.Utils;

public class VideoPlayActivity extends AppCompatActivity {

    private static final String TAG = "VideoPlayActivity";

    private SurfaceView mSurfaceView;
    private Button mBtnStart;
    private Button mBtnStop;
    private Surface mSurface;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_video_play);
        init();
    }

    private void init() {
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
        new Thread(() -> {
            if (mSurface != null) {
                HWCodec.decode(Utils.getHWRecordOutput(), mSurface, null);
            } else {
                Log.e(TAG, "surface is null");
            }
        }).start();
    }

    private void stop() {

    }

    private class SurfaceCallback implements SurfaceHolder.Callback {

        @Override
        public void surfaceCreated(SurfaceHolder holder) {
            mSurface = holder.getSurface();
        }

        @Override
        public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

        }

        @Override
        public void surfaceDestroyed(SurfaceHolder holder) {

        }

    }

}
