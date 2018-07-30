package com.steven.avgraphics.activity;

import android.content.res.AssetManager;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;

import com.steven.avgraphics.BaseActivity;
import com.steven.avgraphics.R;
import com.steven.avgraphics.util.ToastHelper;
import com.steven.avgraphics.util.Utils;

public class OpenSLActivity extends BaseActivity implements View.OnClickListener {

    private static final String ASSET_MP3_FILE = "opensl.mp3";

    private Button mBtnStartPlayMp3;
    private Button mBtnStopPlayMp3;
    private Button mBtnStartRecord;
    private Button mBtnStopRecord;
    private Button mBtnStartPlayPcm;
    private Button mBtnStopPlayPcm;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_opensl);
        init();
    }

    private void init() {
        findView();
        setListener();
    }

    private void findView() {
        mBtnStartPlayMp3 = findViewById(R.id.opensl_btn_start_play_mp3);
        mBtnStopPlayMp3 = findViewById(R.id.opensl_btn_stop_play_mp3);
        mBtnStartRecord = findViewById(R.id.opensl_btn_start_record);
        mBtnStopRecord = findViewById(R.id.opensl_btn_stop_record);
        mBtnStartPlayPcm = findViewById(R.id.opensl_btn_start_play_pcm);
        mBtnStopPlayPcm = findViewById(R.id.opensl_btn_stop_play_pcm);
    }

    private void setListener() {
        mBtnStartPlayMp3.setOnClickListener(this);
        mBtnStopPlayMp3.setOnClickListener(this);
        mBtnStartRecord.setOnClickListener(this);
        mBtnStopRecord.setOnClickListener(this);
        mBtnStartPlayPcm.setOnClickListener(this);
        mBtnStopPlayPcm.setOnClickListener(this);
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.opensl_btn_start_play_mp3:
                startPlayMp3();
                break;
            case R.id.opensl_btn_stop_play_mp3:
                stopPlayMp3();
                break;
            case R.id.opensl_btn_start_record:
                startRecord();
                break;
            case R.id.opensl_btn_stop_record:
                stopRecord();
                break;
            case R.id.opensl_btn_start_play_pcm:
                startPlayPcm();
                break;
            case R.id.opensl_btn_stop_play_pcm:
                stopPlayPcm();
                break;
        }
    }

    private void startPlayMp3() {
        _startPlayMp3(getAssets(), ASSET_MP3_FILE);
        disableButtons();
        mBtnStopPlayMp3.setEnabled(true);
    }

    private void stopPlayMp3() {
        _stopPlayMp3();
        resetButtons();
    }

    private void startRecord() {
        if (_startRecord(Utils.getOpenSLOutput())) {
            disableButtons();
            mBtnStopRecord.postDelayed(() -> mBtnStopRecord.setEnabled(true), 1000);
        } else {
            ToastHelper.show(R.string.opensl_msg_init_recorder_failed);
        }
    }

    private void stopRecord() {
        _stopRecord();
        resetButtons();
    }

    private void startPlayPcm() {
        _startPlayPcm(Utils.getOpenSLOutput());
        disableButtons();
        mBtnStopPlayPcm.setEnabled(true);
    }

    private void stopPlayPcm() {
        _stopPlayPcm();
        resetButtons();
    }

    private void disableButtons() {
        mBtnStartPlayMp3.setEnabled(false);
        mBtnStopPlayMp3.setEnabled(false);
        mBtnStartRecord.setEnabled(false);
        mBtnStopRecord.setEnabled(false);
        mBtnStartPlayPcm.setEnabled(false);
        mBtnStopPlayPcm.setEnabled(false);
    }

    private void resetButtons() {
        mBtnStartPlayMp3.setEnabled(true);
        mBtnStopPlayMp3.setEnabled(false);
        mBtnStartRecord.setEnabled(true);
        mBtnStopRecord.setEnabled(false);
        mBtnStartPlayPcm.setEnabled(true);
        mBtnStopPlayPcm.setEnabled(false);
    }

    @Override
    protected void onPause() {
        super.onPause();
        _stopPlayMp3();
        _stopPlayPcm();
        _stopRecord();
    }

    private static native void _startPlayMp3(AssetManager assetManager, String filename);

    private static native void _stopPlayMp3();

    private static native boolean _startRecord(String filePath);

    private static native void _stopRecord();

    private static native void _startPlayPcm(String filePath);

    private static native void _stopPlayPcm();

}
