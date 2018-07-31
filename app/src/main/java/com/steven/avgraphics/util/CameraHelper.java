package com.steven.avgraphics.util;


import android.app.Activity;
import android.content.pm.PackageManager;
import android.hardware.Camera;
import android.util.Log;
import android.view.Surface;

import com.steven.avgraphics.BaseApplication;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

@SuppressWarnings("WeakerAccess")
public class CameraHelper {

    private static final String TAG = "CameraHelper";

    public static Camera openCamera() {
        return openCamera(Camera.CameraInfo.CAMERA_FACING_BACK);
    }

    public static Camera openCamera(int cameraId) {
        if (!haveFeature(PackageManager.FEATURE_CAMERA)) {
            Log.e(TAG, "no camera!");
            return null;
        }

        if (cameraId == Camera.CameraInfo.CAMERA_FACING_FRONT
                && !haveFeature(PackageManager.FEATURE_CAMERA_FRONT)) {
            Log.e(TAG, "no front camera!");
            return null;
        }

        Camera camera = Camera.open(cameraId);
        if (camera == null) {
            Log.e(TAG, "openCamera failed");
            return null;
        }

        return camera;
    }

    private static boolean haveFeature(String name) {
        return BaseApplication.getContext().getPackageManager().hasSystemFeature(name);
    }

    public static void setFocusMode(Camera camera, String focusMode) {
        Camera.Parameters parameters = camera.getParameters();
        List<String> focusModes = parameters.getSupportedFocusModes();
        if (focusModes.contains(focusMode)) {
            parameters.setFocusMode(focusMode);
        }
        camera.setParameters(parameters);
    }

    public static void setDisplayOritation(Activity activity, Camera camera, int cameraId) {
        int rotation = activity.getWindowManager().getDefaultDisplay().getRotation();
        int degress = 0;
        switch (rotation) {
            case Surface.ROTATION_0:
                degress = 0;
                break;
            case Surface.ROTATION_90:
                degress = 90;
                break;
            case Surface.ROTATION_180:
                degress = 180;
                break;
            case Surface.ROTATION_270:
                degress = 270;
                break;
        }

        Camera.CameraInfo info = new Camera.CameraInfo();
        Camera.getCameraInfo(cameraId, info);
        int result;
        if (info.facing == Camera.CameraInfo.CAMERA_FACING_FRONT) {
            result = (info.orientation + degress) % 360;
            result = (360 - result) % 360;  // compensate the mirror
        } else {
            result = (info.orientation - degress + 360) % 360; // back-facing
        }
        Log.d(TAG, "window rotation: " + degress + ", camera oritation: " + result);
        camera.setDisplayOrientation(result);
    }

    public static boolean isFacingBack(int cameraId) {
        Camera.CameraInfo info = new Camera.CameraInfo();
        Camera.getCameraInfo(cameraId, info);
        return info.facing == Camera.CameraInfo.CAMERA_FACING_BACK;
    }

    public static void releaseCamera(Camera camera) {
        if (camera != null) {
            camera.setPreviewCallback(null);
            camera.stopPreview();
            camera.release();
        }
    }

    public static void setOptimalSize(Camera camera, float aspectRatio, int maxWidth, int maxHeight) {
        Camera.Parameters parameters = camera.getParameters();
        Camera.Size size = CameraHelper.chooseOptimalSize(parameters.getSupportedPreviewSizes(),
                aspectRatio, maxWidth, maxHeight);
        parameters.setPreviewSize(size.width, size.height);
        Log.d(TAG,  "input max: (" + maxWidth + ", " + maxHeight + "), output size: ("
                + size.width + ", " + size.height + ")");
        camera.setParameters(parameters);
    }

    public static Camera.Size chooseOptimalSize(List<Camera.Size> options, float aspectRatio,
                                                int maxWidth, int maxHeight) {
        List<Camera.Size> alternative = new ArrayList<>();
        for (Camera.Size option : options) {
            if (option.height == option.width * aspectRatio && option.width <= maxWidth
                    && option.height <= maxHeight) {
                alternative.add(option);
            }
        }

        if (alternative.size() > 0) {
            return Collections.max(alternative, new CompareSizesByArea());
        }

        return options.get(0);
    }

    private static class CompareSizesByArea implements Comparator<Camera.Size> {

        @Override
        public int compare(Camera.Size lhs, Camera.Size rhs) {
            // 转型 long 是为了确保乘法运算不会溢出
            return Long.signum((long) lhs.width * lhs.height - (long) rhs.width * rhs.height);
        }

    }


}
