package com.steven.avgraphics.util;


import android.content.pm.PackageManager;
import android.hardware.Camera;
import android.util.Log;

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

    public static Camera.Size chooseCameraSize(List<Camera.Size> options, int width, int height) {
        List<Camera.Size> bigEnough = new ArrayList<>();
        List<Camera.Size> equalRatio = new ArrayList<>();
        for (Camera.Size option : options) {
            if (option.height == option.width * height / width) {
                equalRatio.add(option);
                if (option.width >= width && option.height >= height) {
                    bigEnough.add(option);
                }
            }
        }

        if (bigEnough.size() > 0) {
            return Collections.min(bigEnough, new CompareSizesByArea());
        } else if (equalRatio.size() > 0) {
            return Collections.max(equalRatio, new CompareSizesByArea());
        } else {
            Log.w(TAG, "Couldn't find any suitable preview size for: (" + width + ", " + height + ")");
            return options.get(0);
        }
    }

    private static class CompareSizesByArea implements Comparator<Camera.Size> {

        @Override
        public int compare(Camera.Size lhs, Camera.Size rhs) {
            // 转型 long 是为了确保乘法运算不会溢出
            return Long.signum((long) lhs.width * lhs.height - (long) rhs.width * rhs.height);
        }

    }


}
