package com.steven.avgraphics.ui;

import android.Manifest;
import android.content.Context;
import android.content.pm.PackageManager;
import android.graphics.SurfaceTexture;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.TotalCaptureResult;
import android.hardware.camera2.params.StreamConfigurationMap;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.support.annotation.NonNull;
import android.support.annotation.RequiresApi;
import android.support.v4.app.ActivityCompat;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.util.Size;
import android.view.Surface;
import android.view.TextureView;
import android.view.Window;
import android.view.WindowManager;

import com.steven.avgraphics.R;
import com.steven.avgraphics.util.Camera2Helper;
import com.steven.avgraphics.util.ToastHelper;

import java.util.Collections;

@RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
public class Camera2Activity extends AppCompatActivity {

    private static final String TAG = "Camera2Activity";

//    private static final float[] ASCPECT_RATIO_ARRAY = {9.0f / 16, 3.0f / 4};

    private TextureView mTextureView;

    private CameraDevice mCameraDevice;
    private CaptureRequest mCaptureRequest;
    private CaptureRequest.Builder mCaptureRequestBuilder;
    private CameraCaptureSession mCaptureSession;

    private HandlerThread mBackgroundThread;
    private Handler mBackgroundHandler;
    private float mAspectRatio = 9.0f / 16;
    private Size mPreviewSize;
    private String mCameraId;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                WindowManager.LayoutParams.FLAG_FULLSCREEN);
        setContentView(R.layout.activity_camera2);
        init();
    }

    private void init() {
        mTextureView = findViewById(R.id.camera2_texture_view);
    }

    @Override
    protected void onResume() {
        super.onResume();
        startBackgroundThread();
        if (!mTextureView.isAvailable()) {
            mTextureView.setSurfaceTextureListener(getTextureListener());
        } else {
            openCamera(mTextureView.getWidth(), mTextureView.getHeight());
        }
    }

    @Override
    protected void onPause() {
        closeCamera();
        stopBackgroundThread();
        super.onPause();
    }

    private void startBackgroundThread() {
        mBackgroundThread = new HandlerThread("Camera2");
        mBackgroundThread.start();
        mBackgroundHandler = new Handler(mBackgroundThread.getLooper());
    }

    private TextureView.SurfaceTextureListener getTextureListener() {
        return new TextureView.SurfaceTextureListener() {
            @Override
            public void onSurfaceTextureAvailable(SurfaceTexture surface, int width, int height) {
                openCamera(width, height);
            }

            @Override
            public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) {

            }

            @Override
            public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
                return false;
            }

            @Override
            public void onSurfaceTextureUpdated(SurfaceTexture surface) {

            }
        };
    }

    @SuppressWarnings("ConstantConditions")
    private void openCamera(int width, int height) {
        if (ActivityCompat.checkSelfPermission(this, Manifest.permission.CAMERA)
                != PackageManager.PERMISSION_GRANTED) {
            ToastHelper.show(R.string.camera2_msg_no_permission);
            return;
        }

        CameraManager manager = (CameraManager) getSystemService(Context.CAMERA_SERVICE);
        if (manager == null) {
            ToastHelper.show(R.string.camera2_msg_no_camera_service);
            return;
        }
        try {
            for (String cameraId : manager.getCameraIdList()) {
                CameraCharacteristics characteristics = manager.getCameraCharacteristics(cameraId);
                Integer facing = characteristics.get(CameraCharacteristics.LENS_FACING);
                if (facing == null || facing == CameraCharacteristics.LENS_FACING_FRONT) {
                    continue;
                }
                StreamConfigurationMap map = characteristics.get(
                        CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
                mPreviewSize = Camera2Helper.chooseOptimalSize(
                        map.getOutputSizes(SurfaceTexture.class), mAspectRatio, width, height);
                mCameraId = cameraId;
                break;
            }
            manager.openCamera(mCameraId, getStateCallback(), null);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    private CameraDevice.StateCallback getStateCallback() {
        return new CameraDevice.StateCallback() {
            @Override
            public void onOpened(@NonNull CameraDevice camera) {
                mCameraDevice = camera;
                startPreview();
            }

            @Override
            public void onDisconnected(@NonNull CameraDevice camera) {
                camera.close();
                mCameraDevice = null;
            }

            @Override
            public void onError(@NonNull CameraDevice camera, int error) {
                Log.e(TAG, "open camera error: " + error);
                ToastHelper.showOnUiThread(R.string.camera2_msg_open_error);
                camera.close();
                mCameraDevice = null;
                finish();
            }
        };
    }

    private void startPreview() {
        SurfaceTexture texture = mTextureView.getSurfaceTexture();
        texture.setDefaultBufferSize(mPreviewSize.getWidth(), mPreviewSize.getHeight());
        Surface surface = new Surface(texture);
        try {
            mCaptureRequestBuilder = mCameraDevice.createCaptureRequest(
                    CameraDevice.TEMPLATE_PREVIEW);
            mCaptureRequestBuilder.addTarget(surface);
            mCameraDevice.createCaptureSession(Collections.singletonList(surface),
                    new CameraCaptureSession.StateCallback() {
                        @Override
                        public void onConfigured(@NonNull CameraCaptureSession session) {
                            mCaptureRequest = mCaptureRequestBuilder.build();
                            mCaptureSession = session;
                            try {
                                mCaptureSession.setRepeatingRequest(mCaptureRequest,
                                        getCaptureCallback(), mBackgroundHandler);
                            } catch (CameraAccessException e) {
                                e.printStackTrace();
                            }
                        }

                        @Override
                        public void onConfigureFailed(@NonNull CameraCaptureSession session) {
                            Log.e(TAG, "startPreview onConfigureFailed");
                        }
                    }, null);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    private CameraCaptureSession.CaptureCallback getCaptureCallback() {
        return new CameraCaptureSession.CaptureCallback() {

            @Override
            public void onCaptureCompleted(@NonNull CameraCaptureSession session,
                                           @NonNull CaptureRequest request,
                                           @NonNull TotalCaptureResult result) {
                super.onCaptureCompleted(session, request, result);
            }
        };
    }

    private void stopBackgroundThread() {
        mBackgroundThread.quitSafely();
        try {
            mBackgroundThread.join();
            mBackgroundThread = null;
            mBackgroundHandler = null;
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

    private void closeCamera() {
        if (mCaptureSession != null) {
            mCaptureSession.close();
            mCaptureSession = null;
        }

        if (mCameraDevice != null) {
            mCameraDevice.close();
            mCameraDevice = null;
        }
    }

}
