package com.steven.avgraphics.activity;

import android.os.Bundle;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;

import com.steven.avgraphics.BaseActivity;
import com.steven.avgraphics.R;
import com.steven.avgraphics.view.CameraPreviewView;

public class FFmpegActivity extends BaseActivity implements View.OnClickListener {

    private CameraPreviewView mCameraPreviewView;
    private Button mBtnDecode;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                WindowManager.LayoutParams.FLAG_FULLSCREEN);
        setContentView(R.layout.activity_ffmpeg);
        init();
    }

    private void init() {
        mCameraPreviewView = findViewById(R.id.ff_cpv_preview);
        mBtnDecode = findViewById(R.id.ff_btn_decode);
        setListener();
    }

    private void setListener() {
        mBtnDecode.setOnClickListener(this);
    }

    @Override
    protected void onPause() {
        super.onPause();
        mCameraPreviewView.release();
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.ff_btn_decode:
                decode();
                break;
        }
    }

    private void decode() {
        
    }


}
