package com.steven.avgraphics.ui;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioRecord;
import android.media.AudioTrack;
import android.media.MediaRecorder;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.View;
import android.widget.Button;

import com.steven.avgraphics.BaseActivity;
import com.steven.avgraphics.R;
import com.steven.avgraphics.util.ToastHelper;
import com.steven.avgraphics.util.Utils;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;

public class AudioActivity extends BaseActivity implements View.OnClickListener {

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

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_audio);
        init();
    }

    private void init() {
        findView();
        setListener();
        mPcmFile = new File(Utils.getAppDir() + File.separator + FILE_NAME_AUDIO_OUTPUT);
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
        mAudioRecorder.start();
        disableButtons();
        new Handler().postDelayed(() -> mBtnStopRecord.setEnabled(true), 3000);
    }

    private void stopRecord() {
        mAudioRecorder.stop();
        resetButtonState();
    }

    private void startPlay() {
        mAudioPlayer.start();
        disableButtons();
        mBtnStopPlay.setEnabled(true);
    }

    private void stopPlay() {
        mAudioPlayer.stop();
        resetButtonState();
    }

    private void disableButtons() {
        mBtnStartPlay.setEnabled(false);
        mBtnStopPlay.setEnabled(false);
        mBtnStartRecord.setEnabled(false);
        mBtnStopRecord.setEnabled(false);
    }

    private void resetButtonState() {
        mBtnStartPlay.setEnabled(true);
        mBtnStopPlay.setEnabled(false);
        mBtnStartRecord.setEnabled(true);
        mBtnStopRecord.setEnabled(false);
    }

    @Override
    protected void onPause() {
        super.onPause();
        mAudioPlayer.stop();
        mAudioRecorder.stop();
    }

    private class AudioRecorder {

        private ExecutorService mExecutor = Executors.newCachedThreadPool();
        private AudioRecord mAudioRecord;
        private int mBufferSize;
        private BlockingQueue<byte[]> mAudioDataQueue = new LinkedBlockingQueue<>();
        private boolean mIsRecording = false;

        private void start() {
            try {
                mBufferSize = AudioRecord.getMinBufferSize(mSampleRate, AudioFormat.CHANNEL_IN_MONO,
                        mPcmDataFormat);
                mAudioRecord = new AudioRecord(MediaRecorder.AudioSource.MIC, mSampleRate,
                        AudioFormat.CHANNEL_IN_MONO, mPcmDataFormat, mBufferSize);
            } catch (Exception e) {
                Log.e(TAG, "init AudioRecord exception: " + e.getLocalizedMessage());
                return;
            }

            if (mAudioRecord.getState() != AudioRecord.STATE_INITIALIZED) {
                Log.e(TAG, "cannot init AudioRecord");
                return;
            }
            mIsRecording = true;
            mExecutor.execute(this::record);
            mExecutor.execute(this::writeAudioData);
        }

        private void record() {
            android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_URGENT_AUDIO);
            if (mAudioRecord == null || mAudioRecord.getState() != AudioRecord.STATE_INITIALIZED) {
                return;
            }

            ByteBuffer audioBuffer = ByteBuffer.allocate(mBufferSize);
            mAudioRecord.startRecording();
            Log.d(TAG, "record started");

            int readResult;
            while (mIsRecording) {
                readResult = mAudioRecord.read(audioBuffer.array(), 0, mBufferSize);
                if (readResult > 0) {
                    audioBuffer.limit(readResult);
                    mAudioDataQueue.add(audioBuffer.array());
                }
            }

            releaseAudioRecord();
            Log.d(TAG, "record finished");
        }

        private void writeAudioData() {
            if (mPcmFile.exists() && !mPcmFile.delete()) {
                Log.e(TAG, "delete pcm failed");
            }

            DataOutputStream dos;
            try {
                dos = new DataOutputStream(new FileOutputStream(mPcmFile));
            } catch (FileNotFoundException e) {
                e.printStackTrace();
                return;
            }
            Log.d(TAG, "writeAudioData started");

            while (mIsRecording || !mAudioDataQueue.isEmpty()) {
                try {
                    byte[] audioData = mAudioDataQueue.poll(500, TimeUnit.MILLISECONDS);
                    dos.write(audioData);
                } catch (InterruptedException | IOException e) {
                    e.printStackTrace();
                    break;
                }
            }

            try {
                dos.flush();
                dos.close();
            } catch (IOException e) {
                e.printStackTrace();
            }

            Log.d(TAG, "writeAudioData finished");
        }

        private void stop() {
            mIsRecording = false;
            try {
                mExecutor.shutdown();
                mExecutor.awaitTermination(500, TimeUnit.MILLISECONDS);
                releaseAudioRecord();
            } catch (InterruptedException e) {
                Log.e(TAG, "stop exception occur: " + e.getMessage());
            }
            mAudioDataQueue.clear();
        }

        private void releaseAudioRecord() {
            if (mAudioRecord != null) {
                mAudioRecord.stop();
                mAudioRecord.release();
                mAudioRecord = null;
            }
        }

    }

    private class AudioPlayer {

        private AudioTrack mAudioTrack;
        private volatile boolean mIsPlaying = false;
        private int mBufferSize;
        private ExecutorService mExecutor = Executors.newSingleThreadExecutor();

        private void start() {
            if (!mPcmFile.exists()) {
                ToastHelper.show(R.string.audio_msg_no_audio_file);
                return;
            }
            if (mIsPlaying) {
                ToastHelper.show(R.string.audio_msg_playing_now);
                return;
            }
            releaseAudioTrack();
            mBufferSize = AudioTrack.getMinBufferSize(mSampleRate, AudioFormat.CHANNEL_OUT_MONO,
                    mPcmDataFormat);
            mAudioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, mSampleRate,
                    AudioFormat.CHANNEL_OUT_MONO, mPcmDataFormat, mBufferSize, AudioTrack.MODE_STREAM);
            mIsPlaying = true;
            mExecutor.execute(this::play);
        }

        private void play() {
            Log.d(TAG, "play started");
            try {
                byte[] buffer = new byte[mBufferSize];
                int readCount;
                DataInputStream dis = new DataInputStream(new FileInputStream(mPcmFile));
                while (dis.available() > 0 && mIsPlaying) {
                    readCount = dis.read(buffer);
                    if (readCount == AudioTrack.ERROR_INVALID_OPERATION || readCount == AudioTrack.ERROR_BAD_VALUE) {
                        continue;
                    }
                    if (readCount > 0) {
                        mAudioTrack.play();
                        mAudioTrack.write(buffer, 0, readCount);
                    }
                }
                mIsPlaying = false;
            } catch (IOException e) {
                mIsPlaying = false;
                Log.e(TAG, "read audio data fail: " + e.getMessage());
            }
            Log.d(TAG, "play stopped");
        }

        private void stop() {
            mIsPlaying = false;
            try {
                mExecutor.shutdown();
                mExecutor.awaitTermination(500, TimeUnit.MILLISECONDS);
                releaseAudioTrack();
            } catch (InterruptedException e) {
                Log.e(TAG, "stop play faild");
            }
            resetButtonState();
        }

        private void releaseAudioTrack() {
            if (mAudioTrack != null) {
                mAudioTrack.stop();
                mAudioTrack.release();
                mAudioTrack = null;
            }
        }

    }


}
