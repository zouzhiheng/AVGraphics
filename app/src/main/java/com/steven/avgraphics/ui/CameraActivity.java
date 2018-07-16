package com.steven.avgraphics.ui;

import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;

import com.steven.avgraphics.R;
import com.steven.avgraphics.view.CameraPreviewView;

public class CameraActivity extends AppCompatActivity {

    private CameraPreviewView mCameraPreviewView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_camera);

        mCameraPreviewView = findViewById(R.id.camera_cpv_preview);
        findViewById(R.id.camera_btn_switch_camera).setOnClickListener(v -> mCameraPreviewView.switchCamera());
    }
}
