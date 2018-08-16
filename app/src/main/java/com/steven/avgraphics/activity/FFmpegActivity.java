package com.steven.avgraphics.activity;

import android.hardware.Camera;
import android.media.AudioFormat;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;

import com.steven.avgraphics.BaseActivity;
import com.steven.avgraphics.R;
import com.steven.avgraphics.module.av.AudioRecorder;
import com.steven.avgraphics.module.av.FFCodec;
import com.steven.avgraphics.util.ToastHelper;
import com.steven.avgraphics.util.Utils;
import com.steven.avgraphics.view.CameraPreviewView;

import java.io.File;

public class FFmpegActivity extends BaseActivity implements View.OnClickListener,
        Camera.PreviewCallback, CameraPreviewView.PreviewCallback,
        AudioRecorder.AudioRecordCallback {

    private CameraPreviewView mCameraPreviewView;
    private Button mBtnDecode;
    private Button mBtnTranscode;
    private Button mBtnStartRecord;
    private Button mBtnStopRecord;

    private AudioRecorder mAudioRecorder = new AudioRecorder();
    private int mImageWidth;
    private int mImageHeight;

    private boolean mIsReocrding = false;

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
        findView();
        setListener();
    }

    private void findView() {
        mCameraPreviewView = findViewById(R.id.ff_cpv_preview);
        mBtnDecode = findViewById(R.id.ff_btn_decode);
        mBtnTranscode = findViewById(R.id.ff_btn_transcode);
        mBtnStartRecord = findViewById(R.id.ff_btn_start_record);
        mBtnStopRecord = findViewById(R.id.ff_btn_stop_record);
    }

    private void setListener() {
        mBtnDecode.setOnClickListener(this);
        mBtnTranscode.setOnClickListener(this);
        mBtnStartRecord.setOnClickListener(this);
        mBtnStopRecord.setOnClickListener(this);
        mCameraPreviewView.setPreviewCallback(this);
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
            case R.id.ff_btn_transcode:
                transcode();
                break;
            case R.id.ff_btn_start_record:
                startRecord();
                break;
            case R.id.ff_btn_stop_record:
                stopRecord();
                break;
        }
    }

    private void decode() {
        File file = new File(Utils.getFFRecordOutput());
        if (!file.exists()) {
            ToastHelper.show(R.string.ff_msg_no_file);
            return;
        }
        FFCodec._decode(Utils.getFFRecordOutput(), Utils.getFFDecodeYuvOutput(),
                Utils.getFFDecodePcmOutput());
        disableButtons();
        Handler handler = new Handler(Looper.getMainLooper());
        // 假设解码需要 3s
        handler.postDelayed(this::resetButtons, 3000);
    }

    private void transcode() {
        File file = new File(Utils.getFFRecordOutput());
        if (!file.exists()) {
            ToastHelper.show(R.string.ff_msg_no_file);
            return;
        }
        FFCodec.TranscodeParams params = new FFCodec.TranscodeParams(Utils.getFFRecordOutput(),
                Utils.getFFTranscodeOutput());
        params.setReencode(true);
        FFCodec.transcode(params, succeed -> {
            Utils.runOnUiThread(this::resetButtons);
            ToastHelper.showOnUiThread(succeed ? R.string.ff_msg_transcode_succeed : R.string.ff_msg_transcode_failed);
        });
        disableButtons();
    }

    private void startRecord() {
        mAudioRecorder.setPcmFormat(AudioFormat.ENCODING_PCM_16BIT);
        mAudioRecorder.setRecordCallback(this);

        mImageWidth = mCameraPreviewView.getImageWidth();
        mImageHeight = mCameraPreviewView.getImageHeight();

        FFCodec.RecordParams params = new FFCodec.RecordParams(Utils.getFFRecordOutput(),
                mImageWidth, mImageHeight, 24, mAudioRecorder.getSampleRate(),
                FFCodec.PIXEL_FORMAT_NV21, FFCodec.SAMPLE_FORMAT_16BIT,
                mAudioRecorder.getChannels());
        FFCodec.FilterParams filter = new FFCodec.FilterParams();
        filter.setRotate(90);
        params.setFilterParams(filter);
        mIsReocrding = FFCodec.initRecorder(params,
                succeed -> ToastHelper.showOnUiThread(R.string.ff_msg_record_ok));
        if (mIsReocrding) {
            mAudioRecorder.start();
            disableButtons();
            Handler handler = new Handler(Looper.getMainLooper());
            // 最少录制 3s 视频
            handler.postDelayed(() -> mBtnStopRecord.setEnabled(true), 3000);
        } else {
            ToastHelper.show(R.string.ff_msg_init_recorder_failed);
        }
    }

    private void stopRecord() {
        mIsReocrding = false;
        mAudioRecorder.stop();
        FFCodec.stopRecord();
        resetButtons();
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
        camera.setPreviewCallback(this);
    }

    @Override
    public void onPreviewStopped() {

    }

    @Override
    public void onPreviewFailed() {
        finishWithToast(R.string.ff_msg_preview_failed);
    }

    @Override
    public void onPreviewFrame(byte[] data, Camera camera) {
        if (mIsReocrding) {
            FFCodec.recordImage(data, data.length, mImageWidth, mImageHeight,
                    FFCodec.PIXEL_FORMAT_NV21);
        }
    }

    @Override
    public void onRecordSample(byte[] data) {
        if (mIsReocrding) {
            FFCodec.recordSample(data, data.length);
        }
    }

}
