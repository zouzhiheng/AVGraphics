package com.steven.avgraphics.activity.gles;

import android.content.pm.ActivityInfo;
import android.content.res.AssetManager;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.Window;
import android.view.WindowManager;

import com.steven.avgraphics.BaseActivity;
import com.steven.avgraphics.R;
import com.steven.avgraphics.util.CameraHelper;
import com.steven.avgraphics.util.ToastHelper;
import com.steven.avgraphics.util.Utils;

import java.io.IOException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class GLCameraActivity extends BaseActivity {

    private static final float ASPECT_RATIO = 16.0f / 9;

    private Camera mCamera;
    private SurfaceTexture mSurfaceTexture;
    private float[] mMatrix = new float[16];
    private ExecutorService mExecutor = Executors.newSingleThreadExecutor();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
        setContentView(R.layout.activity_glcamera);
        init();
    }

    private void init() {
        openCamera();
        SurfaceView surfaceView = findViewById(R.id.glcam_sv_window);
        surfaceView.getHolder().addCallback(new SurfaceCallback());
    }

    private void openCamera() {
        if (mCamera != null) {
            return;
        }
        mCamera = CameraHelper.openCamera();
        if (mCamera == null) {
            ToastHelper.show(R.string.glcam_msg_open_camera_failed);
            finish();
        }

        CameraHelper.setOptimalSize(mCamera, ASPECT_RATIO, Utils.getScreenWidth(), Utils.getScreenWidth());
        mCamera.setDisplayOrientation(90);
    }

    @Override
    protected void onResume() {
        super.onResume();
        openCamera();
    }

    @Override
    protected void onPause() {
        super.onPause();
        CameraHelper.releaseCamera(mCamera);
        mCamera = null;
    }

    private class SurfaceCallback implements SurfaceHolder.Callback {

        @Override
        public void surfaceCreated(SurfaceHolder holder) {

        }

        @Override
        public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
            initOpenGL(holder.getSurface(), width, height);
        }

        @Override
        public void surfaceDestroyed(SurfaceHolder holder) {
            releaseOpenGL();
        }

        private void initOpenGL(Surface surface, int width, int height) {
            mExecutor.execute(() -> {
                int textureId = _init(surface, width, height, getAssets());
                if (textureId < 0) {
                    Log.e(TAG, "surfaceCreated init OpenGL ES failed!");
                    return;
                }
                mSurfaceTexture = new SurfaceTexture(textureId);
                mSurfaceTexture.setOnFrameAvailableListener(surfaceTexture -> drawOpenGL());
                try {
                    mCamera.setPreviewTexture(mSurfaceTexture);
                    mCamera.startPreview();
                } catch (IOException e) {
                    Log.e(TAG, "onSurfaceCreated exception: " + e.getLocalizedMessage());
                }
            });
        }

        private void drawOpenGL() {
            mExecutor.execute(() -> {
                if (mSurfaceTexture != null) {
                    mSurfaceTexture.updateTexImage(); // 必须运行在 OpenGL 线程环境中
                    mSurfaceTexture.getTransformMatrix(mMatrix);
                    _draw(mMatrix);
                }
            });
        }

        private void releaseOpenGL() {
            mExecutor.execute(() -> {
                if (mSurfaceTexture != null) {
                    mSurfaceTexture.release();
                    mSurfaceTexture = null;
                }
                _release();
            });
        }

    }

    private static native int _init(Surface surface, int width, int height, AssetManager manager);

    private static native void _draw(float[] matrix);

    private static native void _release();
}
