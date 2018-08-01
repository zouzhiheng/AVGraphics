package com.steven.avgraphics.activity.gles;

import android.content.pm.ActivityInfo;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.SeekBar;

import com.steven.avgraphics.R;
import com.steven.avgraphics.view.CameraPreviewView;

public class BeautyActivity extends AppCompatActivity {

    private LinearLayout mLlControlBar;
    private CameraPreviewView mCameraPreviewView;
    private SeekBar mSbBeauty;
    private SeekBar mSbSaturate;
    private SeekBar mSbBright;
    private Button mBtnSwitchCamera;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
        setContentView(R.layout.activity_beauty);
        init();
    }

    private void init() {
        findView();
        setListener();
        mLlControlBar.setVisibility(mCameraPreviewView.isFacingBack() ? View.GONE : View.VISIBLE);
    }

    private void findView() {
        mLlControlBar = findViewById(R.id.beauty_ll_control_bar);
        mCameraPreviewView = findViewById(R.id.beauty_cpv_window);
        mSbBeauty = findViewById(R.id.beauty_sb_beauty);
        mSbSaturate = findViewById(R.id.beauty_sb_saturate);
        mSbBright = findViewById(R.id.beauty_sb_bright);
        mBtnSwitchCamera = findViewById(R.id.beauty_btn_switch_camera);
    }

    private void setListener() {
        mSbBeauty.setOnSeekBarChangeListener((DefaultSeekBarChangeListener) progress ->
                mCameraPreviewView.setBeautyLevel(progress));
        mSbSaturate.setOnSeekBarChangeListener((DefaultSeekBarChangeListener) progress ->
                mCameraPreviewView.setSaturateLevel(progress));
        mSbBright.setOnSeekBarChangeListener((DefaultSeekBarChangeListener) progress ->
                mCameraPreviewView.setBrightLevel(progress));

        mBtnSwitchCamera.setOnClickListener(v -> {
            mCameraPreviewView.switchCamera();
            mLlControlBar.setVisibility(mCameraPreviewView.isFacingBack() ? View.GONE : View.VISIBLE);
        });
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
