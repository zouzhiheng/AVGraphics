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
import android.widget.LinearLayout;
import android.widget.SeekBar;

import com.steven.avgraphics.BaseActivity;
import com.steven.avgraphics.R;
import com.steven.avgraphics.module.av.AVInfo;
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
    private Button mBtnSwitch;
    private LinearLayout mLlSeerBarContainer;
    private SeekBar mSbBeauty;
    private SeekBar mSbSaturate;
    private SeekBar mSbBright;

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
        mBtnSwitch = findViewById(R.id.ff_btn_switch_camera);
        mLlSeerBarContainer = findViewById(R.id.ff_ll_seekbar_container);
        mSbBeauty = findViewById(R.id.ff_sb_beauty);
        mSbSaturate = findViewById(R.id.ff_sb_saturate);
        mSbBright = findViewById(R.id.ff_sb_bright);
    }

    private void setListener() {
        mBtnDecode.setOnClickListener(this);
        mBtnTranscode.setOnClickListener(this);
        mBtnStartRecord.setOnClickListener(this);
        mBtnStopRecord.setOnClickListener(this);
        mBtnSwitch.setOnClickListener(this);
        mCameraPreviewView.setPreviewCallback(this);

        mSbBeauty.setOnSeekBarChangeListener((DefaultSeekBarChangeListener) progress
                -> mCameraPreviewView.setBeautyLevel(progress));
        mSbSaturate.setOnSeekBarChangeListener((DefaultSeekBarChangeListener) progress
                -> mCameraPreviewView.setSaturateLevel(progress));
        mSbBright.setOnSeekBarChangeListener((DefaultSeekBarChangeListener) progress
                -> mCameraPreviewView.setBrightLevel(progress));
    }

    @Override
    protected void onPause() {
        super.onPause();
        mCameraPreviewView.release();
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.ff_btn_switch_camera:
                switchCamera();
                break;
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

    private void switchCamera() {
        mCameraPreviewView.switchCamera();
        mLlSeerBarContainer.setVisibility(mCameraPreviewView.isFacingBack() ? View.INVISIBLE : View.VISIBLE);
    }

    private void decode() {
        File file = new File(Utils.getFFRecordOutput());
        if (!file.exists()) {
            ToastHelper.show(R.string.ff_msg_no_file);
            return;
        }
        FFCodec.decode(Utils.getFFRecordOutput(), Utils.getFFDecodeYuvOutput(),
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
                AVInfo.PIXEL_FORMAT_NV21, AVInfo.SAMPLE_FORMAT_16BIT,
                mAudioRecorder.getChannels());
        FFCodec.FilterParams filter = new FFCodec.FilterParams();
        // 由于 OpenGL 和 Android 坐标系的不同，使用美颜时需要倒转 180 度才能得到正确的画面
        filter.setRotate(mCameraPreviewView.isFacingBack() ? 90 : 180);
        filter.setVideoFilter("drawbox=x=100:y=100:w=100:h=100:color=pink@0.5");
        params.setFilterParams(filter);
        mIsReocrding = FFCodec.initRecorder(params,
                succeed -> ToastHelper.showOnUiThread(R.string.ff_msg_record_ok));
        if (mIsReocrding) {
            mAudioRecorder.start();
            mCameraPreviewView.setRecording(true);
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
        mCameraPreviewView.setRecording(false);
        FFCodec.stopRecord();
        resetButtons();
    }

    private void disableButtons() {
        mBtnDecode.setEnabled(false);
        mBtnTranscode.setEnabled(false);
        mBtnStartRecord.setEnabled(false);
        mBtnStopRecord.setEnabled(false);
        mBtnSwitch.setEnabled(false);
    }

    private void resetButtons() {
        mBtnDecode.setEnabled(true);
        mBtnTranscode.setEnabled(true);
        mBtnStartRecord.setEnabled(true);
        mBtnStopRecord.setEnabled(false);
        mBtnSwitch.setEnabled(true);
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
        if (mIsReocrding && !mCameraPreviewView.isBeautyOpen()) {
            FFCodec.recordImage(data, data.length, mImageWidth, mImageHeight,
                    AVInfo.PIXEL_FORMAT_NV21);
        }
    }

    @Override
    public void onRecordSample(byte[] data) {
        if (mIsReocrding) {
            FFCodec.recordSample(data, data.length);
        }
    }

    private interface DefaultSeekBarChangeListener extends SeekBar.OnSeekBarChangeListener {

        void onProgressChanged(float progress);

        @Override
        default void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
            onProgressChanged(1.0f * progress / 100);
        }

        @Override
        default void onStartTrackingTouch(SeekBar seekBar) {

        }

        @Override
        default void onStopTrackingTouch(SeekBar seekBar) {

        }
    }

}
