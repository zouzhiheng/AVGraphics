package com.steven.avgraphics.activity;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioRecord;
import android.media.AudioTrack;
import android.os.Bundle;
import android.os.Handler;
import android.support.annotation.NonNull;
import android.util.Log;
import android.view.View;
import android.widget.Button;

import com.steven.avgraphics.BaseActivity;
import com.steven.avgraphics.R;
import com.steven.avgraphics.module.av.AudioRecorder;
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

public class AudioActivity extends BaseActivity implements View.OnClickListener,
        AudioRecorder.AudioRecordCallback {

    private static final int DEFAULT_SAMPLE_RATE = 48000;
    private static final int DEFAULT_PCM_DATA_FORMAT = AudioFormat.ENCODING_PCM_16BIT;
    private static final int DEFAULT_CHANNELS = 1;

    private Button mBtnStartRecord;
    private Button mBtnStopRecord;
    private Button mBtnStartPlay;
    private Button mBtnStopPlay;

    private AudioPlayer mAudioPlayer = new AudioPlayer();
    private AudioRecorder mAudioRecorder = new AudioRecorder();
    private File mPcmFile;
    private int mSampleRate = DEFAULT_SAMPLE_RATE;
    private int mPcmDataFormat = DEFAULT_PCM_DATA_FORMAT;
    private int mChannels = DEFAULT_CHANNELS;
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
        mPcmFile = new File(Utils.getAudioOutputPcm());
        mAudioRecorder.setSampleRate(mSampleRate);
        mAudioRecorder.setPcmFormat(mPcmDataFormat);
        mAudioRecorder.setChannels(mChannels);
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
        pcmToWav(mPcmFile, new File(Utils.getAudioOutputWav()));
    }

    private void pcmToWav(File pcmFile, File wavFile) {
        FileInputStream fis = null;
        FileOutputStream fos = null;
        try {
            fis = new FileInputStream(pcmFile);
            fos = new FileOutputStream(wavFile);

            int sampleFormat = mPcmDataFormat == AudioFormat.ENCODING_PCM_16BIT ? 16 : 8;
            writeWavHeader(fos, fis.getChannel().size(), sampleFormat, mSampleRate, mChannels);

            int channelConfig =mChannels == 1 ? AudioFormat.CHANNEL_IN_MONO : AudioFormat.CHANNEL_IN_STEREO;
            int bufferSize = AudioRecord.getMinBufferSize(mSampleRate, channelConfig, mPcmDataFormat);

            byte[] data = new byte[bufferSize];
            while (fis.read(data) != -1) {
                fos.write(data);
            }
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            try {
                if (fis != null) {
                    fis.close();
                }

                if (fos != null) {
                    fos.flush();
                    fos.close();
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    private void writeWavHeader(@NonNull FileOutputStream fos, long pcmDataLength, int sampleFormat,
                                int sampleRate, int channels) throws IOException {
        long audioDataLength = pcmDataLength + 36;
        long bitRate = sampleRate * channels * sampleFormat / 8;
        byte[] header = new byte[44];
        // RIFF
        header[0] = 'R';
        header[1] = 'I';
        header[2] = 'F';
        header[3] = 'F';
        // pcm data length
        header[4] = (byte) (pcmDataLength & 0xff);
        header[5] = (byte) ((pcmDataLength >> 8) & 0xff);
        header[6] = (byte) ((pcmDataLength >> 16) & 0xff);
        header[7] = (byte) ((pcmDataLength >> 24) & 0xff);
        // WAVE
        header[8] = 'W';
        header[9] = 'A';
        header[10] = 'V';
        header[11] = 'E';
        // 'fmt '
        header[12] = 'f';
        header[13] = 'm';
        header[14] = 't';
        header[15] = ' ';
        header[16] = 16;
        header[17] = 0;
        header[18] = 0;
        header[19] = 0;
        // 1(PCM)
        header[20] = 1;
        header[21] = 0;
        // channels
        header[22] = (byte) channels;
        header[23] = 0;
        // sample rate
        header[24] = (byte) (sampleRate & 0xff);
        header[25] = (byte) ((sampleRate >> 8) & 0xff);
        header[26] = (byte) ((sampleRate >> 16) & 0xff);
        header[27] = (byte) ((sampleRate >> 24) & 0xff);
        // bit rate
        header[28] = (byte) (bitRate & 0xff);
        header[29] = (byte) ((bitRate >> 8) & 0xff);
        header[30] = (byte) ((bitRate >> 16) & 0xff);
        header[31] = (byte) ((bitRate >> 24) & 0xff);
        header[32] = 4;
        header[33] = 0;
        // 采样精度
        header[34] = (byte) sampleFormat;
        header[35] = 0;
        // data
        header[36] = 'd';
        header[37] = 'a';
        header[38] = 't';
        header[39] = 'a';
        // data length
        header[40] = (byte) (audioDataLength & 0xff);
        header[41] = (byte) ((audioDataLength >> 8) & 0xff);
        header[42] = (byte) ((audioDataLength >> 16) & 0xff);
        header[43] = (byte) ((audioDataLength >> 24) & 0xff);
        fos.write(header);
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
            int channelConfig = mChannels == 1 ? AudioFormat.CHANNEL_OUT_MONO : AudioFormat.CHANNEL_OUT_STEREO;
            mBufferSize = AudioTrack.getMinBufferSize(mSampleRate, channelConfig, mPcmDataFormat);
            mAudioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, mSampleRate, channelConfig,
                    mPcmDataFormat, mBufferSize, AudioTrack.MODE_STREAM);
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
