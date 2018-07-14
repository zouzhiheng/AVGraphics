package com.steven.avgraphics.ui;

import android.hardware.Camera;
import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;

import com.steven.avgraphics.BaseActivity;
import com.steven.avgraphics.R;
import com.steven.avgraphics.util.CameraHelper;
import com.steven.avgraphics.util.HWCodec;
import com.steven.avgraphics.util.ToastHelper;
import com.steven.avgraphics.util.Utils;

import java.io.IOException;

public class HWRecordActivity extends BaseActivity implements View.OnClickListener,
        Camera.PreviewCallback, SurfaceHolder.Callback {

    private static final String TAG = "HWRecordActivity";

    private static final int DEFAULT_BITRATE = 10 * 1000 * 1000;

    private SurfaceView mSurfaceView;
    private Button mBtnStartRecord;
    private Button mBtnStopRecord;

    private HWCodec.RecorderWrapper mRecorder = new HWCodec.RecorderWrapper();
    private Camera mCamera;
    private Camera.Size mPreviewSize;
    private int mPrevieweFormat;
    private int mChannels = 1;
    private int mSampleRate = 48000;
    private volatile boolean mIsRecording = false;
    private final Object mLockObject = new Object();

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
    }

    private void findView() {
        mSurfaceView = findViewById(R.id.hwrecord_sv_preview);
        mBtnStartRecord = findViewById(R.id.hwrecord_btn_start_record);
        mBtnStopRecord = findViewById(R.id.hwrecord_btn_stop_record);
    }

    private void setListener() {
        mBtnStartRecord.setOnClickListener(this);
        mBtnStopRecord.setOnClickListener(this);
        mSurfaceView.getHolder().addCallback(this);
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
            new Thread(new AudioRecordRunnable()).start();
            mIsRecording = true;
        } else {
            ToastHelper.show("无法创建硬件编码器");
        }
    }

    private void stopRecord() {
        if (mIsRecording) {
            synchronized (mLockObject) {
                mIsRecording = false;
            }
            mBtnStartRecord.setEnabled(true);
            mBtnStopRecord.setEnabled(false);
            mRecorder.release();
            ToastHelper.show("视频正在处理中，请稍后...");
        }
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {

    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        openCamera(holder, width, height);
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        stopRecord();
        releaseCamera();
    }

    private void openCamera(SurfaceHolder holder, int width, int height) {
        if (mCamera != null) {
            return;
        }
        mCamera = CameraHelper.openCamera();
        if (mCamera == null) {
            finish();
        }

        Camera.Parameters parameters = mCamera.getParameters();
        mPreviewSize = CameraHelper.chooseCameraSize(parameters.getSupportedPreviewSizes(), width, height);
        parameters.setPreviewSize(mPreviewSize.width, mPreviewSize.height);
        mPrevieweFormat = parameters.getPreviewFormat();
        mCamera.setParameters(parameters);
        mCamera.setDisplayOrientation(90);
        mCamera.setPreviewCallback(this);
        try {
            mCamera.setPreviewDisplay(holder);
            mCamera.startPreview();
        } catch (IOException e) {
            Log.e(TAG, "openCamera preview failed: " + e.getLocalizedMessage());
            ToastHelper.show("相机预览开启失败！");
            releaseCamera();
            finish();
        }
    }

    private void releaseCamera() {
        if (mCamera != null) {
            mCamera.setPreviewCallback(null);
            mCamera.stopPreview();
            mCamera.release();
            mCamera = null;
        }
    }

    @Override
    public void onPreviewFrame(byte[] data, Camera camera) {
        if (mIsRecording) {
            mRecorder.recordImage(data);
        }
    }

    private class AudioRecordRunnable implements Runnable {

        private AudioRecord mAudioRecord;
        private int mBufferSize;

        private AudioRecordRunnable() {
            try {
                mChannels = 1;
                mBufferSize = AudioRecord.getMinBufferSize(mSampleRate, AudioFormat.CHANNEL_IN_MONO,
                        AudioFormat.ENCODING_PCM_16BIT);
                mAudioRecord = new AudioRecord(MediaRecorder.AudioSource.MIC, mSampleRate,
                        AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT, mBufferSize);
            } catch (Exception e) {
                Log.e(TAG, "init AudioRecord exception: " + e.getLocalizedMessage());
            }

            if (mAudioRecord == null || mAudioRecord.getState() != AudioRecord.STATE_INITIALIZED) {
                try {
                    mSampleRate = 16000;
                    mChannels = 2;
                    mBufferSize = AudioRecord.getMinBufferSize(mSampleRate, AudioFormat.CHANNEL_IN_STEREO,
                            AudioFormat.ENCODING_PCM_16BIT);
                    mAudioRecord = new AudioRecord(MediaRecorder.AudioSource.MIC, mSampleRate,
                            AudioFormat.CHANNEL_IN_STEREO, AudioFormat.ENCODING_PCM_16BIT, mBufferSize);
                } catch (Exception e) {
                    Log.e(TAG, "init AudioRecord exception: " + e.getLocalizedMessage());
                }
            }

            if (mAudioRecord == null || mAudioRecord.getState() != AudioRecord.STATE_INITIALIZED) {
                Log.e(TAG, "cannot init AudioRecord");
            }
        }

        @Override
        public void run() {
            android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_URGENT_AUDIO);
            if (mAudioRecord == null || mAudioRecord.getState() != AudioRecord.STATE_INITIALIZED) {
                return;
            }

            byte[] audioData = new byte[mBufferSize];
            mAudioRecord.startRecording();
            Log.d(TAG, "AudioRecord started");

            int actualSize;
            while (mIsRecording) {
                actualSize = mAudioRecord.read(audioData, 0, audioData.length);
                synchronized (mLockObject) {
                    if (actualSize > 0 && mIsRecording) {
                        mRecorder.recordSample(audioData);
                    }
                }
            }

            if (mAudioRecord != null) {
                mAudioRecord.stop();
                mAudioRecord.release();
                mAudioRecord = null;
                Log.d(TAG, "AudioRecord released");
            }
        }
    }

}

