package com.steven.avgraphics.activity;

import android.app.ActivityManager;
import android.content.Context;
import android.content.pm.ConfigurationInfo;
import android.os.Bundle;

import com.steven.avgraphics.BaseActivity;
import com.steven.avgraphics.R;
import com.steven.avgraphics.activity.gles.EGLCircleActivity;
import com.steven.avgraphics.activity.gles.JavaTriangleActivity;
import com.steven.avgraphics.activity.gles.JniTriangleActivity;
import com.steven.avgraphics.activity.gles.ShapeActivity;

public class OpenGLActivity extends BaseActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_opengl);
        init();
    }

    private void init() {
        findViewById(R.id.opengl_btn_java_triangle).setOnClickListener(
                v -> startActivity(JavaTriangleActivity.class));
        findViewById(R.id.opengl_btn_jni_triangle).setOnClickListener(
                v -> startActivity(JniTriangleActivity.class));
        findViewById(R.id.opengl_btn_egl_circle).setOnClickListener(
                v -> ShapeActivity.start(OpenGLActivity.this, ShapeActivity.SHAPE_CIRCLE));
        findViewById(R.id.opengl_btn_square).setOnClickListener(
                v -> ShapeActivity.start(OpenGLActivity.this, ShapeActivity.SHAPE_SQUARE));
    }

    @Override
    protected void onResume() {
        super.onResume();
        ActivityManager activityManager = (ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);
        if (activityManager != null) {
            ConfigurationInfo configurationInfo = activityManager.getDeviceConfigurationInfo();
            boolean supportedEs3 = configurationInfo.reqGlEsVersion >= 0x30000;
            if (!supportedEs3) {
                finishWithToast(R.string.opengl_msg_not_support);
            }
        }
    }

}
