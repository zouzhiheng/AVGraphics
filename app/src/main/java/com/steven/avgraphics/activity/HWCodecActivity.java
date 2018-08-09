package com.steven.avgraphics.activity;

import android.hardware.Camera;
import android.os.Bundle;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;

import com.steven.avgraphics.BaseActivity;
import com.steven.avgraphics.R;
import com.steven.avgraphics.module.av.AudioRecorder;
import com.steven.avgraphics.module.av.HWCodec;
import com.steven.avgraphics.module.av.HWDecoder;
import com.steven.avgraphics.module.av.HWRecorderWrapper;
import com.steven.avgraphics.util.ToastHelper;
import com.steven.avgraphics.util.Utils;
import com.steven.avgraphics.view.CameraPreviewView;

import java.io.File;

public class HWCodecActivity extends BaseActivity implements View.OnClickListener,
        Camera.PreviewCallback, CameraPreviewView.PreviewCallback, AudioRecorder.AudioRecordCallback {

    private static final int DEFAULT_BITRATE = 10 * 1000 * 1000;

    private Button mBtnDecode;
    private Button mBtnTranscode;
    private Button mBtnStartRecord;
    private Button mBtnStopRecord;

    private HWRecorderWrapper mRecorder = new HWRecorderWrapper();
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
        setContentView(R.layout.activity_hwcodec);
        init();
    }

    private void init() {
        findView();
        setListener();
        initData();
    }

    private void findView() {
        mCameraPreviewView = findViewById(R.id.hwcodec_cpv_preview);
        mBtnDecode = findViewById(R.id.hwcodec_btn_decode);
        mBtnTranscode = findViewById(R.id.hwcodec_btn_transcode);
        mBtnStartRecord = findViewById(R.id.hwcodec_btn_start_record);
        mBtnStopRecord = findViewById(R.id.hwcodec_btn_stop_record);
    }

    private void setListener() {
        mCameraPreviewView.setPreviewCallback(this);
        mBtnDecode.setOnClickListener(this);
        mBtnTranscode.setOnClickListener(this);
        mBtnStartRecord.setOnClickListener(this);
        mBtnStopRecord.setOnClickListener(this);
    }

    private void initData() {
        mAudioRecorder.setSampleRate(mSampleRate);
        mAudioRecorder.setRecordCallback(this);
        mChannels = mAudioRecorder.getChannels();
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (mIsRecording) {
            stopRecord();
        }
        mCameraPreviewView.release();
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.hwcodec_btn_decode:
                decode();
                break;
            case R.id.hwcodec_btn_transcode:
                transcode();
                break;
            case R.id.hwcodec_btn_start_record:
                startRecord();
                break;
            case R.id.hwcodec_btn_stop_record:
                stopRecord();
                break;
        }
    }

    private void decode() {
        if (!new File(Utils.getHWRecordOutput()).exists()) {
            ToastHelper.show(R.string.hwcodec_msg_no_video);
            return;
        }
        disableButtons();
        new Thread(() -> {
            HWDecoder decoder = new HWDecoder();
            decoder.start(Utils.getHWRecordOutput(), Utils.getHWDecodeYuvOutput(),
                    Utils.getHWDecodePcmOutput(), (vsucceed, asucceed) -> {
                        ToastHelper.showOnUiThread(vsucceed && asucceed
                                ? R.string.hwcodec_msg_decode_succeed
                                : R.string.hwcodec_msg_decode_failed);
                        Utils.runOnUiThread(this::resetButtons);
                    });
        }).start();
    }

    private void transcode() {
        if (!new File(Utils.getHWRecordOutput()).exists()) {
            ToastHelper.show(R.string.hwcodec_msg_no_video);
            return;
        }
        disableButtons();
        new Thread(() -> {
            boolean succeed = HWCodec.transcode(Utils.getHWRecordOutput(), Utils.getHWTranscodeOutput());
            ToastHelper.showOnUiThread(succeed ? R.string.hwcodec_msg_transcode_succeed : R.string.hwcodec_msg_transcode_failed);
            Utils.runOnUiThread(this::resetButtons);
        }).start();
    }

    private void startRecord() {
        boolean succeed = mRecorder.init(mPreviewSize.width, mPreviewSize.height, mPrevieweFormat,
                DEFAULT_BITRATE, mSampleRate, mChannels, Utils.getHWRecordOutput());
        if (succeed) {
            disableButtons();
            mBtnStopRecord.postDelayed(() -> mBtnStopRecord.setEnabled(true), 3000);
            mIsRecording = true;
            mAudioRecorder.start();
        } else {
            ToastHelper.show(R.string.hwcodec_msg_create_codec_failed);
        }
    }

    private void stopRecord() {
        mIsRecording = false;
        resetButtons();
        mAudioRecorder.stop();
        mRecorder.stop();
        ToastHelper.show(R.string.hwcodec_msg_video_is_processing);
    }

    private void disableButtons() {
        mBtnDecode.setEnabled(false);
        mBtnTranscode.setEnabled(false);
        mBtnStartRecord.setEnabled(false);
        mBtnStopRecord.setEnabled(false);
    }

    private void resetButtons() {
        mBtnDecode.setEnabled(true);
        mBtnTranscode.setEnabled(true);
        mBtnStartRecord.setEnabled(true);
        mBtnStopRecord.setEnabled(false);
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
        ToastHelper.show(R.string.hwcodec_msg_preview_failed);
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
