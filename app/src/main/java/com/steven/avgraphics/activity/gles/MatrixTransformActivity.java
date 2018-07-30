package com.steven.avgraphics.activity.gles;

import android.opengl.Matrix;
import android.os.Bundle;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import com.steven.avgraphics.BaseActivity;
import com.steven.avgraphics.R;
import com.steven.avgraphics.util.Utils;

public class MatrixTransformActivity extends BaseActivity {

    private static final float PI = 3.14159265358979f;

    private float[] mMvpMatrix = new float[16];
    private float[] mDrawMatrix = new float[16];

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_matrix_transform);
        init();
    }

    private void init() {
        Matrix.setIdentityM(mDrawMatrix, 0);
        initSurfaceView();
        setListener();
    }

    private void initSurfaceView() {
        SurfaceView surfaceView = findViewById(R.id.mxtsf_sv_window);
        surfaceView.getLayoutParams().width = Utils.getScreenWidth();
        surfaceView.getLayoutParams().height = Utils.getScreenWidth();
        surfaceView.getHolder().addCallback(new SurfaceCallback());
    }

    private void setListener() {
        findViewById(R.id.mxtsf_btn_magnify).setOnClickListener(v -> {
            scale(mDrawMatrix, 1.2f);
            _draw(mDrawMatrix);
        });

        findViewById(R.id.mxtsf_btn_minify).setOnClickListener(v -> {
            scale(mDrawMatrix, 1.0f / 1.2f);
            _draw(mDrawMatrix);
        });

        findViewById(R.id.mxtsf_btn_right_translate).setOnClickListener(v -> {
            translate(mDrawMatrix, 0.1f, 0.0f, 0.0f);
            _draw(mDrawMatrix);
        });

        findViewById(R.id.mxtsf_btn_left_translate).setOnClickListener(v -> {
            translate(mDrawMatrix, -0.1f, 0.0f, 0.0f);
            _draw(mDrawMatrix);
        });

        findViewById(R.id.mxtsf_btn_left_rotate).setOnClickListener(v -> {
            rotate(mDrawMatrix, 15);
            _draw(mDrawMatrix);
        });

        findViewById(R.id.mxtsf_btn_right_rotate).setOnClickListener(v -> {
            rotate(mDrawMatrix, -15);
            _draw(mDrawMatrix);
        });

        findViewById(R.id.mxtsf_btn_mvp).setOnClickListener(v -> {
            float[] tmp = mDrawMatrix.clone();
            Matrix.multiplyMM(mDrawMatrix, 0, tmp, 0, mMvpMatrix, 0);
            _draw(mDrawMatrix);
        });
    }

    // Android 本身提供了矩阵的运算 API
    // 以下几个方法只是为了验证所学的矩阵知识，可能写得不够准确，但测试能达到预期的效果
    private void scale(float[] matrix, float factor) {
        float[] scaleMatrix = new float[16];
        scaleMatrix[0] = factor;
        scaleMatrix[5] = factor;
        scaleMatrix[10] = factor;
        scaleMatrix[15] = 1.0f;

        float[] tmp = matrix.clone();
        Matrix.multiplyMM(matrix, 0, tmp, 0, scaleMatrix, 0);
    }

    @SuppressWarnings("SameParameterValue")
    private void translate(float[] matrix, float tx, float ty, float tz) {
        matrix[12] += (matrix[0] * tx + matrix[4] * ty + matrix[8] * tz);
        matrix[13] += (matrix[1] * tx + matrix[5] * ty + matrix[9] * tz);
        matrix[14] += (matrix[2] * tx + matrix[6] * ty + matrix[10] * tz);
        matrix[15] += (matrix[3] * tx + matrix[7] * ty + matrix[11] * tz);
    }

    // 沿 z 轴旋转
    private void rotate(float[] matrix, double angle) {
        double radian = 1.0 * angle * PI / 180;
        float sin = (float) Math.sin(radian);
        float cos = (float) Math.cos(radian);

        float[] rotateMatrix = new float[16];
        rotateMatrix[0] = cos;
        rotateMatrix[1] = sin;
        rotateMatrix[4] = -sin;
        rotateMatrix[5] = cos;
        rotateMatrix[10] = 1.0f;
        rotateMatrix[15] = 1.0f;

        float[] tmp = matrix.clone();
        Matrix.multiplyMM(matrix, 0, tmp, 0, rotateMatrix, 0);
    }

    private class SurfaceCallback implements SurfaceHolder.Callback {

        @Override
        public void surfaceCreated(SurfaceHolder holder) {

        }

        @Override
        public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
            float ratio = (float) width / height;
            // 观察矩阵
            float[] viewMatrix = new float[16];
            // 投影矩阵
            float[] projectMatrix = new float[16];
            // 定义左右上下近远平面，在这个空间之外的物体会被截掉
            Matrix.frustumM(projectMatrix, 0, -ratio, ratio, -1, 1,
                    3, 7);
            // 定义摄像机位置、摄像机看向的方向、上向量的方向(一般为 y 轴，即(0.0, 1.0, 0.0))
            Matrix.setLookAtM(viewMatrix, 0, 0, 0, 5.0f,
                    0f, 0f, 0f, 0f, 1.0f, 0.0f);
            Matrix.multiplyMM(mMvpMatrix, 0, projectMatrix, 0,
                    viewMatrix, 0);

            _init(holder.getSurface(), width, height);
            _draw(mDrawMatrix);
        }

        @Override
        public void surfaceDestroyed(SurfaceHolder holder) {
            _release();
        }

    }

    private static native void _init(Surface surface, int width, int height);

    private static native void _draw(float[] matrix);

    private static native void _release();

}
