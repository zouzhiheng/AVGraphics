package com.steven.avgraphics.util;


import android.os.Environment;
import android.util.DisplayMetrics;

import com.steven.avgraphics.BaseApplication;

import java.io.File;

public class Utils {

    private static final String SDCARD = Environment.getExternalStorageDirectory().getAbsolutePath();
    private static final String APP_DIR = SDCARD + "/AVGraphics";
    private static final String CAMERA_OUTPUT = APP_DIR + "/camera.yuv";
    private static final String HWRECORD_OUTPUT = APP_DIR + "/hwrecord.mp4";
    private static final String HWDECODE_YUV_OUTPUT = APP_DIR + "/hwdecode.yuv";
    private static final String HWDECODE_PCM_OUTPUT = APP_DIR + "/hwdecode.pcm";
    private static final String HWTRANSCODE_OUTPUT = APP_DIR + "/hwtranscode.mp4";

    static {
        createDir(APP_DIR);
    }

    public static String getAppDir() {
        return APP_DIR;
    }

    public static String getHWRecordOutput() {
        return HWRECORD_OUTPUT;
    }

    public static String getHWDecodeYuvOutput() {
        return HWDECODE_YUV_OUTPUT;
    }

    public static String getHWDecodePcmOutput() {
        return HWDECODE_PCM_OUTPUT;
    }

    public static String getHWTranscodeOutput() {
        return HWTRANSCODE_OUTPUT;
    }

    public static String getCameraOutput() {
        return CAMERA_OUTPUT;
    }

    private static void createDir(String path) {
        File file = new File(path);
        if (!file.exists() && !file.mkdir()) {
            ToastHelper.show("文件夹创建过程中出现错误: " + path);
        }
    }

    public static int getScreenWidth() {
        DisplayMetrics metrics = BaseApplication.getContext().getResources().getDisplayMetrics();
        return metrics.widthPixels;
    }

    public static int getScreenHeight() {
        DisplayMetrics metrics = BaseApplication.getContext().getResources().getDisplayMetrics();
        return metrics.heightPixels;
    }

}
