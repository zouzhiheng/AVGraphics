package com.steven.avgraphics.activity;

import android.os.Bundle;

import com.steven.avgraphics.BaseActivity;
import com.steven.avgraphics.R;
import com.steven.avgraphics.activity.gles.ShapeActivity;
import com.steven.avgraphics.activity.gles.TriangleActivity;

public class OpenGLActivity extends BaseActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_opengl);
        init();
    }

    private void init() {
        findViewById(R.id.opengl_btn_triangle).setOnClickListener(
                v -> startActivity(TriangleActivity.class));
        findViewById(R.id.opengl_btn_shape).setOnClickListener(
                v -> startActivity(ShapeActivity.class));
    }

}
