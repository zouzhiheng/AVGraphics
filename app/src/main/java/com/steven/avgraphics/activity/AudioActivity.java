package com.steven.avgraphics.activity;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.View;
import android.widget.Button;

import com.steven.avgraphics.BaseActivity;
import com.steven.avgraphics.R;
import com.steven.avgraphics.util.AudioRecorder;
import com.steven.avgraphics.util.ToastHelper;
import com.steven.avgraphics.util.Utils;

import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

public class AudioActivity extends BaseActivity implements View.OnClickListener, AudioRecorder.AudioRecordCallback {

    private static final String FILE_NAME_AUDIO_OUTPUT = "audio_record.pcm";

    private static final int DEFAULT_SAMPLE_RATE = 48000;
    private static final int DEFAULT_PCM_DATA_FORMAT = AudioFormat.ENCODING_PCM_8BIT;

    private Button mBtnStartRecord;
    private Button mBtnStopRecord;
    private Button mBtnStartPlay;
    private Button mBtnStopPlay;

    private AudioPlayer mAudioPlayer = new AudioPlayer();
    private AudioRecorder mAudioRecorder = new AudioRecorder();
    private File mPcmFile;
    private int mSampleRate = DEFAULT_SAMPLE_RATE;
    private int mPcmDataFormat = DEFAULT_PCM_DATA_FORMAT;
    private FileOutputStream mFileOutputStream;
    private boolean mIsRecording = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_audio);
        init();
    }

    private void init() {
        findView();
        setListener();
        initData();
    }

    private void findView() {
        mBtnStartRecord = findViewById(R.id.audio_btn_start_record);
        mBtnStopRecord = findViewById(R.id.audio_btn_stop_record);
        mBtnStartPlay = findViewById(R.id.audio_btn_start_play);
        mBtnStopPlay = findViewById(R.id.audio_btn_stop_play);
    }

    private void setListener() {
        mBtnStartRecord.setOnClickListener(this);
        mBtnStopRecord.setOnClickListener(this);
        mBtnStartPlay.setOnClickListener(this);
        mBtnStopPlay.setOnClickListener(this);
    }

    private void initData() {
        mPcmFile = new File(Utils.getAppDir() + File.separator + FILE_NAME_AUDIO_OUTPUT);
        mAudioRecorder.setSampleRate(mSampleRate);
        mAudioRecorder.setPcmFormat(mPcmDataFormat);
        mAudioRecorder.setRecordCallback(this);
    }

    @Override
    protected void onPause() {
        super.onPause();
        mAudioPlayer.stop();
        mAudioRecorder.stop();
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.audio_btn_start_record:
                startRecord();
                break;
            case R.id.audio_btn_stop_record:
                stopRecord();
                break;
            case R.id.audio_btn_start_play:
                startPlay();
                break;
            case R.id.audio_btn_stop_play:
                stopPlay();
                break;
        }
    }

    private void startRecord() {
        if (mAudioRecorder.start()) {
            try {
                mFileOutputStream = new FileOutputStream(mPcmFile);
            } catch (FileNotFoundException e) {
                e.printStackTrace();
            }
            mIsRecording = true;
            disableButtons();
            new Handler().postDelayed(() -> mBtnStopRecord.setEnabled(true), 3000);
        }
    }

    private void stopRecord() {
        mIsRecording = false;
        mAudioRecorder.stop();
        resetButtons();
        try {
            mFileOutputStream.flush();
            mFileOutputStream.close();
        } catch (IOException e) {
            Log.e(TAG, "onRecordEnded exception occur: " + e.getMessage());
        }
    }

    private void startPlay() {
        mAudioPlayer.start();
        disableButtons();
        mBtnStopPlay.setEnabled(true);
    }

    private void stopPlay() {
        mAudioPlayer.stop();
        resetButtons();
    }

    private void disableButtons() {
        mBtnStartPlay.setEnabled(false);
        mBtnStopPlay.setEnabled(false);
        mBtnStartRecord.setEnabled(false);
        mBtnStopRecord.setEnabled(false);
    }

    private void resetButtons() {
        mBtnStartPlay.setEnabled(true);
        mBtnStopPlay.setEnabled(false);
        mBtnStartRecord.setEnabled(true);
        mBtnStopRecord.setEnabled(false);
    }

    @Override
    public void onRecordSample(byte[] data) {
        if (mIsRecording) {
            try {
                mFileOutputStream.write(data);
            } catch (IOException e) {
                Log.e(TAG, "onRecordSample write data failed: " + e.getMessage());
            }
        }
    }

    private class AudioPlayer {

        private AudioTrack mAudioTrack;
        private volatile boolean mIsPlaying = false;
        private int mBufferSize;
        private ExecutorService mExecutor;

        private void start() {
            if (!mPcmFile.exists()) {
                ToastHelper.show(R.string.audio_msg_no_audio_file);
                return;
            }
            if (mIsPlaying) {
                ToastHelper.show(R.string.audio_msg_playing_now);
                return;
            }
            release();
            mBufferSize = AudioTrack.getMinBufferSize(mSampleRate, AudioFormat.CHANNEL_OUT_MONO,
                    mPcmDataFormat);
            mAudioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, mSampleRate,
                    AudioFormat.CHANNEL_OUT_MONO, mPcmDataFormat, mBufferSize, AudioTrack.MODE_STREAM);
            mIsPlaying = true;
            mExecutor = Executors.newSingleThreadExecutor();
            mExecutor.execute(this::play);
        }

        private void play() {
            Log.d(TAG, "AudioPlayer started");
            try {
                byte[] buffer = new byte[mBufferSize];
                int readCount;
                DataInputStream dis = new DataInputStream(new FileInputStream(mPcmFile));
                while (dis.available() > 0 && mIsPlaying) {
                    readCount = dis.read(buffer);
                    if (readCount < 0) {
                        continue;
                    }
                    mAudioTrack.play();
                    mAudioTrack.write(buffer, 0, readCount);
                }
            } catch (Exception e) {
                Log.e(TAG, "play audio failed: " + e.getMessage());
            }
            stop();
            Log.d(TAG, "AudioPlayer stopped");
        }

        private void stop() {
            mIsPlaying = false;
            if (mExecutor != null) {
                try {
                    mExecutor.shutdown();
                    mExecutor.awaitTermination(50, TimeUnit.MILLISECONDS);
                } catch (InterruptedException e) {
                    Log.e(TAG, "stop play faild");
                }
            }
            release();
            Utils.runOnUiThread(AudioActivity.this::resetButtons);
        }

        private void release() {
            if (mAudioTrack != null) {
                mAudioTrack.stop();
                mAudioTrack.release();
                mAudioTrack = null;
            }
        }

    }


}
