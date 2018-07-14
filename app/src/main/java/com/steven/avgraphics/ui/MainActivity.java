package com.steven.avgraphics.ui;

import android.Manifest;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.view.View;
import android.widget.Button;

import com.steven.avgraphics.BaseActivity;
import com.steven.avgraphics.BaseApplication;
import com.steven.avgraphics.R;
import com.steven.avgraphics.util.ToastHelper;

import java.util.ArrayList;
import java.util.List;

public class MainActivity extends BaseActivity {

    private static final int RC_PERMISSION = 1;

    private Button mBtnToDrawImage;
    private Button mBtnToAudio;
    private Button mBtnHWCodec;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        init();
    }

    private void init() {
        findView();
        setListener();
    }

    private void findView() {
        mBtnToDrawImage = findViewById(R.id.main_btn_draw_image);
        mBtnToAudio = findViewById(R.id.main_btn_audio);
        mBtnHWCodec = findViewById(R.id.main_btn_hwcodec);
    }

    private void setListener() {
        mBtnToDrawImage.setOnClickListener(v -> startActivity(DrawImageActivity.class));
        mBtnToAudio.setOnClickListener(v -> startActivity(AudioActivity.class));
        mBtnHWCodec.setOnClickListener(v -> startActivity(HWCodecActivity.class));
    }

    @Override
    protected void onResume() {
        super.onResume();
        requestPermissions();
    }

    private void requestPermissions() {
        String[] permissions = {
                Manifest.permission.CAMERA,
                Manifest.permission.RECORD_AUDIO,
                Manifest.permission.WRITE_EXTERNAL_STORAGE,
                Manifest.permission.READ_EXTERNAL_STORAGE
        };
        List<String> list = new ArrayList<>();
        for (String permission : permissions) {
            if (ContextCompat.checkSelfPermission(BaseApplication.getContext(), permission)
                    != PackageManager.PERMISSION_GRANTED) {
                list.add(permission);
            }
        }
        String[] requestList = new String[list.size()];
        for (int i = 0; i < list.size(); ++i) {
            requestList[i] = list.get(i);
        }
        if (requestList.length > 0) {
            ActivityCompat.requestPermissions(this, requestList, RC_PERMISSION);
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions,
                                           @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        for (int result : grantResults) {
            if (result != PackageManager.PERMISSION_GRANTED) {
                ToastHelper.show("权限不足，无法运行");
                finish();
            }
        }
    }

    static {
        System.loadLibrary("native-lib");
    }
}
