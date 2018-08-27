package com.steven.avgraphics.activity;

import android.app.ActivityManager;
import android.content.Context;
import android.content.pm.ConfigurationInfo;
import android.os.Bundle;

import com.steven.avgraphics.BaseActivity;
import com.steven.avgraphics.R;
import com.steven.avgraphics.activity.gles.BeautyActivity;
import com.steven.avgraphics.activity.gles.TextureImageActivity;
import com.steven.avgraphics.activity.gles.WatermarkActivity;
import com.steven.avgraphics.activity.gles.EGLCircleActivity;
import com.steven.avgraphics.activity.gles.FboActivity;
import com.steven.avgraphics.activity.gles.GLCameraActivity;
import com.steven.avgraphics.activity.gles.JavaTriangleActivity;
import com.steven.avgraphics.activity.gles.JniTriangleActivity;
import com.steven.avgraphics.activity.gles.MatrixTransformActivity;
import com.steven.avgraphics.activity.gles.VaoVboActivity;

public class OpenGLActivity extends BaseActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_opengl);
        init();
    }

    private void init() {
        findViewById(R.id.opengl_btn_java_triangle).setOnClickListener(v -> startActivity(JavaTriangleActivity.class));
        findViewById(R.id.opengl_btn_jni_triangle).setOnClickListener(v -> startActivity(JniTriangleActivity.class));
        findViewById(R.id.opengl_btn_egl_circle).setOnClickListener(v -> startActivity(EGLCircleActivity.class));
        findViewById(R.id.opengl_btn_vao_vbo).setOnClickListener(v -> startActivity(VaoVboActivity.class));
        findViewById(R.id.opengl_btn_matrix_transform).setOnClickListener(v -> startActivity(MatrixTransformActivity.class));
        findViewById(R.id.opengl_btn_texture_image).setOnClickListener(v -> startActivity(TextureImageActivity.class));
        findViewById(R.id.opengl_btn_fbo).setOnClickListener(v -> startActivity(FboActivity.class));
        findViewById(R.id.opengl_btn_glcamera).setOnClickListener(v -> startActivity(GLCameraActivity.class));
        findViewById(R.id.opengl_btn_camera_effect).setOnClickListener(v -> startActivity(WatermarkActivity.class));
        findViewById(R.id.opengl_btn_beauty).setOnClickListener(v -> startActivity(BeautyActivity.class));
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
