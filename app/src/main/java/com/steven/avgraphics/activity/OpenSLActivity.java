package com.steven.avgraphics.activity;

import android.content.Context;
import android.content.res.AssetManager;
import android.media.AudioManager;
import android.os.Bundle;
import android.util.Log;
import android.widget.Button;

import com.steven.avgraphics.BaseActivity;
import com.steven.avgraphics.R;

public class OpenSLActivity extends BaseActivity {

    private Button mBtnStartPlay;
    private Button mBtnStopPlay;
    private Button mBtnStartRecord;
    private Button mBtnStopRecord;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_opensl);
        init();
    }

    private void init() {
        setupAudioPlayer();
        findView();
        setListener();
    }

    private void setupAudioPlayer() {
        AudioManager manager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
        if (manager == null) {
            Log.e(TAG, "initData failed, cannot find audio service");
            finish();
            return;
        }
        String nativeParam = manager.getProperty(AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE);
        int sampleRate = Integer.parseInt(nativeParam);
        nativeParam = manager.getProperty(AudioManager.PROPERTY_OUTPUT_FRAMES_PER_BUFFER);
        int bufSize = Integer.parseInt(nativeParam);
        _createEngine();
        _createBufferQueueAudioPlayer(sampleRate, bufSize);
        _createAssetAudioPlayer(getAssets(), "background.mp3");
    }

    private void findView() {
        mBtnStartPlay = findViewById(R.id.opensl_btn_start_play);
        mBtnStopPlay = findViewById(R.id.opensl_btn_stop_play);
        mBtnStartRecord = findViewById(R.id.opensl_btn_start_record);
        mBtnStopRecord = findViewById(R.id.opensl_btn_stop_record);
    }

    private void setListener() {
        mBtnStartPlay.setOnClickListener(v -> startPlay());
        mBtnStopPlay.setOnClickListener(v -> stopPlay());
        mBtnStartRecord.setOnClickListener(v -> startRecord());
        mBtnStopRecord.setOnClickListener(v -> stopRecord());
    }

    private void startPlay() {
        _setPlayingAssetAudioPlayer(true);
        disableButtons();
        mBtnStopPlay.setEnabled(true);
    }

    private void stopPlay() {
        _setPlayingAssetAudioPlayer(false);
        resetButtons();
    }

    private void startRecord() {
        if (_createAudioRecorder("/sdcard/opensles.pcm")) {
            _startRecord();
        }
        disableButtons();
        mBtnStopRecord.postDelayed(() -> mBtnStopRecord.setEnabled(true), 1000);
    }

    private void stopRecord() {
        _stopRecord();
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
    protected void onPause() {
        super.onPause();
        _setPlayingAssetAudioPlayer(false);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        _shutdown();
    }

    private static native void _createEngine();

    private static native void _createBufferQueueAudioPlayer(int sampleRate, int bufSize);

    private static native boolean _createAssetAudioPlayer(AssetManager assetManager, String filename);

    // true == PLAYING, false == PAUSED
    private static native void _setPlayingAssetAudioPlayer(boolean isPlaying);

    private static native boolean _createAudioRecorder(String filePath);

    private static native void _startRecord();

    private static native void _stopRecord();

    private static native void _shutdown();
    
}
