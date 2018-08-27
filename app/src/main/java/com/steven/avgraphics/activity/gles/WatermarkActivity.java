package com.steven.avgraphics.activity.gles;

import android.content.pm.ActivityInfo;
import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.opengl.Matrix;
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
import java.nio.ByteBuffer;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class WatermarkActivity extends BaseActivity {

    private static final float ASPECT_RATIO = 16.0f / 9;

    private Camera mCamera;
    private SurfaceTexture mSurfaceTexture;
    private float[] mCameraMatrix = new float[16];
    private float[] mWatermarkMatrix = new float[16];
    private ExecutorService mExecutor = Executors.newSingleThreadExecutor();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
        setContentView(R.layout.activity_watermark);
        init();
    }

    private void init() {
        openCamera();
        Matrix.setIdentityM(mWatermarkMatrix, 0);
        SurfaceView surfaceView = findViewById(R.id.wtmar_sv_window);
        surfaceView.getHolder().addCallback(new SurfaceCallback());
    }

    private void openCamera() {
        if (mCamera != null) {
            return;
        }
        mCamera = CameraHelper.openCamera();
        if (mCamera == null) {
            ToastHelper.show(R.string.wtmar_msg_open_camera_failed);
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

        private byte[] mWatermark;
        private int mWatermarkWidth;
        private int mWatermarkHeight;

        @Override
        public void surfaceCreated(SurfaceHolder holder) {

        }

        @Override
        public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
            makeImageWatermark();
//            makeTextWatermark(width / 2, height / 2);
            initOpenGL(holder.getSurface(), width, height, mWatermark, mWatermarkWidth, mWatermarkHeight);
            drawOpenGL();
        }

        @Override
        public void surfaceDestroyed(SurfaceHolder holder) {
            releaseOpenGL();
        }

        private void makeImageWatermark() {
            BitmapFactory.Options options = new BitmapFactory.Options();
            options.inPreferredConfig = Bitmap.Config.ARGB_8888;
            Bitmap bitmap = BitmapFactory.decodeResource(getResources(), R.mipmap.ic_launcher, options);
            int byteCount = bitmap.getByteCount();
            ByteBuffer buffer = ByteBuffer.allocate(byteCount);
            bitmap.copyPixelsToBuffer(buffer);
            buffer.position(0);
            mWatermark = buffer.array();
            mWatermarkWidth = bitmap.getWidth();
            mWatermarkHeight = bitmap.getHeight();
            bitmap.recycle();

            // 为什么相机纹理和水印纹理坐标的向量方向需要互为 180° 才能得到正确的画面？
            // 为什么旋转后水印消失？
            // 为什么放大=缩小？
            Matrix.scaleM(mWatermarkMatrix, 0, 2.5f, 2.5f, 2.5f);
        }

        private void makeTextWatermark(int width, int height) {
            Bitmap textBitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
            Canvas canvas = new Canvas(textBitmap);
            Paint paint = new Paint();
            paint.setColor(Color.argb(255, 255, 0, 0));
            paint.setTextSize(28);
            paint.setAntiAlias(true);
            paint.setTextAlign(Paint.Align.CENTER);
            Rect rect = new Rect(0, 0, width, height);
            Paint.FontMetricsInt fontMetrics = paint.getFontMetricsInt();
            // 将文字绘制在矩形区域的正中间
            int baseline = (rect.bottom + rect.top - fontMetrics.bottom - fontMetrics.top) / 2;
            canvas.drawText("作者: zouzhiheng", rect.centerX(), baseline, paint);

            int capacity = width * height * 4;
            ByteBuffer buffer = ByteBuffer.allocate(capacity);
            textBitmap.copyPixelsToBuffer(buffer);
            buffer.position(0);
            mWatermark = buffer.array();
            mWatermarkWidth = textBitmap.getWidth();
            mWatermarkHeight = textBitmap.getHeight();
            textBitmap.recycle();

            Matrix.scaleM(mWatermarkMatrix, 0, 2f, 2f, 2f);
        }

        private void initOpenGL(Surface surface, int width, int height, byte[] watermark,
                                int watermarkWidth, int watermarkHeight) {
            mExecutor.execute(() -> {
                int textureId = _init(surface, width, height, watermark, watermark.length,
                        watermarkWidth, watermarkHeight, getAssets());
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
                    mSurfaceTexture.getTransformMatrix(mCameraMatrix);
                    _draw(mCameraMatrix, mWatermarkMatrix);
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

    private static native int _init(Surface surface, int width, int height,
                                    byte[] data, int dataLen, int watermarkWidth,
                                    int watermarkHeight, AssetManager manager);

    private static native void _draw(float[] cameraMatrix, float[] watermarkMatrix);

    private static native void _release();

}
