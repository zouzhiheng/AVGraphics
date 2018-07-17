package com.steven.avgraphics.util;


import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import java.nio.ByteBuffer;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class AudioRecorder {

    private static final String TAG = "AudioRecorder";

    private static final int DEFAULT_SAMPLE_RATE = 48000;
    private static final int DEFAULT_PCM_DATA_FORMAT = AudioFormat.ENCODING_PCM_16BIT;

    private ExecutorService mExecutor = Executors.newCachedThreadPool();
    private AudioRecord mAudioRecord;
    private int mBufferSize;
    private int mSampleRate = DEFAULT_SAMPLE_RATE;
    private int mPcmFormat = DEFAULT_PCM_DATA_FORMAT;

    private AudioRecordCallback mRecordCallback;
    private Handler mHandler;
    private boolean mIsRecording = false;

    public void setSampleRate(int sampleRate) {
        mSampleRate = sampleRate;
    }

    public void setPcmFormat(int pcmFormat) {
        mPcmFormat = pcmFormat;
    }

    public void setRecordCallback(AudioRecordCallback recordCallback) {
        mRecordCallback = recordCallback;
    }

    public int getChannels() {
        return 1;
    }

    public boolean start() {
        try {
            mBufferSize = AudioRecord.getMinBufferSize(mSampleRate, AudioFormat.CHANNEL_IN_MONO,
                    mPcmFormat);
            mAudioRecord = new AudioRecord(MediaRecorder.AudioSource.MIC, mSampleRate,
                    AudioFormat.CHANNEL_IN_MONO, mPcmFormat, mBufferSize);
        } catch (Exception e) {
            Log.e(TAG, "init AudioRecord exception: " + e.getLocalizedMessage());
            return false;
        }

        if (mAudioRecord.getState() != AudioRecord.STATE_INITIALIZED) {
            Log.e(TAG, "cannot init AudioRecord");
            return false;
        }
        mIsRecording = true;
        mExecutor.execute(this::record);
        mHandler = new Handler(Looper.myLooper());

        return true;
    }

    private void record() {
        android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_URGENT_AUDIO);
        if (mAudioRecord == null || mAudioRecord.getState() != AudioRecord.STATE_INITIALIZED) {
            return;
        }

        ByteBuffer audioBuffer = ByteBuffer.allocate(mBufferSize);
        mAudioRecord.startRecording();
        Log.d(TAG, "AudioRecorder started");

        int readResult;
        while (mIsRecording) {
            readResult = mAudioRecord.read(audioBuffer.array(), 0, mBufferSize);
            if (readResult > 0 && mRecordCallback != null) {
                byte[] data = new byte[readResult];
                audioBuffer.position(0);
                audioBuffer.limit(readResult);
                audioBuffer.get(data, 0, readResult);
                mHandler.post(() -> mRecordCallback.onRecordSample(data));
            }
        }

        release();
        Log.d(TAG, "AudioRecorder finished");
    }

    public void stop() {
        mIsRecording = false;
    }

    private void release() {
        if (mAudioRecord != null) {
            mAudioRecord.stop();
            mAudioRecord.release();
            mAudioRecord = null;
        }
    }

    public interface AudioRecordCallback {
        // start 在哪个线程调用，就运行在哪个线程
        void onRecordSample(byte[] data);
    }

}
