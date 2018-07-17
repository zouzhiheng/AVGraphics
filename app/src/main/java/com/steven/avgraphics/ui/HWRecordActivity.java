package com.steven.avgraphics.ui;

import android.hardware.Camera;
import android.os.Bundle;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;

import com.steven.avgraphics.BaseActivity;
import com.steven.avgraphics.R;
import com.steven.avgraphics.util.AudioRecorder;
import com.steven.avgraphics.util.HWCodec;
import com.steven.avgraphics.util.ToastHelper;
import com.steven.avgraphics.util.Utils;
import com.steven.avgraphics.view.CameraPreviewView;

public class HWRecordActivity extends BaseActivity implements View.OnClickListener,
        Camera.PreviewCallback, CameraPreviewView.PreviewCallback, AudioRecorder.AudioRecordCallback {

    private static final int DEFAULT_BITRATE = 10 * 1000 * 1000;

    private Button mBtnStartRecord;
    private Button mBtnStopRecord;

    private HWCodec.RecorderWrapper mRecorder = new HWCodec.RecorderWrapper();
    private AudioRecorder mAudioRecorder = new AudioRecorder();
    private CameraPreviewView mCameraPreviewView;
    private Camera.Size mPreviewSize;
    private int mPrevieweFormat;
    private int mChannels = 1;
    private int mSampleRate = 48000;
    private volatile boolean mIsRecording = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                WindowManager.LayoutParams.FLAG_FULLSCREEN);
        setContentView(R.layout.activity_hwrecord);
        init();
    }

    private void init() {
        findView();
        setListener();
        initData();
    }

    private void findView() {
        mCameraPreviewView = findViewById(R.id.hwrecord_cpv_preview);
        mBtnStartRecord = findViewById(R.id.hwrecord_btn_start_record);
        mBtnStopRecord = findViewById(R.id.hwrecord_btn_stop_record);
    }

    private void setListener() {
        mCameraPreviewView.setPreviewCallback(this);
        mBtnStartRecord.setOnClickListener(this);
        mBtnStopRecord.setOnClickListener(this);
    }

    private void initData() {
        mAudioRecorder.setSampleRate(mSampleRate);
        mAudioRecorder.setRecordCallback(this);
        mChannels = mAudioRecorder.getChannels();
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.hwrecord_btn_start_record:
                startRecord();
                break;
            case R.id.hwrecord_btn_stop_record:
                stopRecord();
                break;
        }
    }

    private void startRecord() {
        boolean succeed = mRecorder.init(mPreviewSize.width, mPreviewSize.height, mPrevieweFormat,
                DEFAULT_BITRATE, mSampleRate, mChannels, Utils.getHWRecordOutput());
        if (succeed) {
            mBtnStartRecord.setEnabled(false);
            mBtnStopRecord.postDelayed(() -> mBtnStopRecord.setEnabled(true), 3000);
            mIsRecording = true;
            mAudioRecorder.start();
        } else {
            ToastHelper.show(R.string.hwrecord_msg_create_codec_failed);
        }
    }

    private void stopRecord() {
        mIsRecording = false;
        mBtnStartRecord.setEnabled(true);
        mBtnStopRecord.setEnabled(false);
        mAudioRecorder.stop();
        mRecorder.stop();
        ToastHelper.show(R.string.hwrecord_msg_video_is_processing);
    }

    @Override
    public void onPreviewStarted(Camera camera) {
        mPreviewSize = camera.getParameters().getPreviewSize();
        mPrevieweFormat = camera.getParameters().getPreviewFormat();
        camera.setPreviewCallback(this);
    }

    @Override
    public void onPreviewStopped() {

    }

    @Override
    public void onPreviewFailed() {
        ToastHelper.show(R.string.hwrecord_msg_preview_failed);
        finish();
    }

    @Override
    public void onPreviewFrame(byte[] data, Camera camera) {
        if (mIsRecording) {
            mRecorder.recordImage(data);
        }
    }

    @Override
    public void onRecordSample(byte[] data) {
        if (mIsRecording) {
            mRecorder.recordSample(data);
        }
    }

}

