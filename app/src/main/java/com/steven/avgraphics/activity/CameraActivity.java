package com.steven.avgraphics.activity;

import android.hardware.Camera;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;

import com.steven.avgraphics.BaseActivity;
import com.steven.avgraphics.R;
import com.steven.avgraphics.util.ToastHelper;
import com.steven.avgraphics.util.Utils;
import com.steven.avgraphics.view.CameraPreviewView;

import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;

public class CameraActivity extends BaseActivity implements View.OnClickListener,
        Camera.PreviewCallback, CameraPreviewView.PreviewCallback  {

    private Button mBtnSwitch;
    private Button mBtnStartGetData;
    private Button mBtnStop;

    private CameraPreviewView mCameraPreviewView;
    private FileOutputStream mFileOutputStream;

    private boolean mIsGettingData = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                WindowManager.LayoutParams.FLAG_FULLSCREEN);
        setContentView(R.layout.activity_camera);
        init();
    }

    private void init() {
        findView();
        setListener();
    }

    private void findView() {
        mCameraPreviewView = findViewById(R.id.camera_cpv_preview);
        mBtnSwitch = findViewById(R.id.camera_btn_switch_camera);
        mBtnStartGetData = findViewById(R.id.camera_btn_start_get_data);
        mBtnStop = findViewById(R.id.camera_btn_stop);
    }

    private void setListener() {
        mCameraPreviewView.setPreviewCallback(this);
        mBtnSwitch.setOnClickListener(this);
        mBtnStartGetData.setOnClickListener(this);
        mBtnStop.setOnClickListener(this);
    }

    @Override
    protected void onPause() {
        super.onPause();
        mCameraPreviewView.release();
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.camera_btn_switch_camera:
                mCameraPreviewView.switchCamera();
                break;
            case R.id.camera_btn_start_get_data:
                startGetData();
                break;
            case R.id.camera_btn_stop:
                stop();
                break;
        }
    }

    private void startGetData() {
        mIsGettingData = true;
        new Handler().postDelayed(() -> mBtnStop.setEnabled(true), 1000);
        mBtnStartGetData.setEnabled(false);
        mBtnSwitch.setEnabled(false);
    }

    private void stop() {
        mIsGettingData = false;
        mBtnStartGetData.setEnabled(true);
        mBtnStop.setEnabled(false);
        mBtnSwitch.setEnabled(true);
    }

    @Override
    public void onPreviewStarted(Camera camera) {
        try {
            mFileOutputStream = new FileOutputStream(Utils.getCameraOutput());
        } catch (FileNotFoundException e) {
            Log.e(TAG, "new FileOutputStream failed: " + e.getMessage());
            ToastHelper.show(R.string.camera_msg_open_file_failed);
            finish();
        }
        camera.setPreviewCallback(this);
    }

    @Override
    public void onPreviewStopped() {

    }

    @Override
    public void onPreviewFailed() {
        ToastHelper.show(R.string.camera_msg_preview_failed);
        finish();
    }

    @Override
    public void onPreviewFrame(byte[] data, Camera camera) {
        if (mIsGettingData) {
            try {
                mFileOutputStream.write(data);
            } catch (IOException e) {
                Log.e(TAG, "onPreviewFrame write data failed: " + e.getMessage());
            }
        }
    }

}
