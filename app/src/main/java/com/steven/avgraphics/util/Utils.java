package com.steven.avgraphics.util;


import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.util.DisplayMetrics;

import com.steven.avgraphics.BaseApplication;

import java.io.File;

@SuppressWarnings("unused")
public class Utils {

    private static final String SDCARD = Environment.getExternalStorageDirectory().getAbsolutePath();
    private static final String APP_DIR = SDCARD + "/AVGraphics";

    private static final String CAMERA_OUTPUT = APP_DIR + "/camera.yuv";
    private static final String CAMERA2_OUTPUT = APP_DIR + "/camera2.jpg";

    private static final String AUDIO_OUTPUT_PCM = APP_DIR + "/audio_record.pcm";
    private static final String AUDIO_OUTPUT_WAV = APP_DIR + "/audio_record.wav";

    private static final String HWRECORD_OUTPUT = APP_DIR + "/hwrecord.mp4";
    private static final String HWDECODE_YUV_OUTPUT = APP_DIR + "/hwdecode.yuv";
    private static final String HWDECODE_PCM_OUTPUT = APP_DIR + "/hwdecode.pcm";
    private static final String HWTRANSCODE_OUTPUT = APP_DIR + "/hwtranscode.mp4";

    private static final String OPENSL_OUTPUT = APP_DIR + "/opensl.pcm";

    private static Handler sHandler = new Handler(Looper.getMainLooper());

    static {
        createDir(APP_DIR);
    }

    private static void createDir(String path) {
        File file = new File(path);
        if (!file.exists() && !file.mkdir()) {
            ToastHelper.show("文件夹创建过程中出现错误: " + path);
        }
    }

    public static String getAppDir() {
        return APP_DIR;
    }

    public static String getCameraOutput() {
        return CAMERA_OUTPUT;
    }

    public static String getCamera2Output() {
        return CAMERA2_OUTPUT;
    }

    public static String getAudioOutputPcm() {
        return AUDIO_OUTPUT_PCM;
    }

    public static String getAudioOutputWav() {
        return AUDIO_OUTPUT_WAV;
    }

    public static String getOpenSLOutput() {
        return OPENSL_OUTPUT;
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

    public static void runOnUiThread(Runnable action) {
        if (Looper.myLooper() == Looper.getMainLooper()) {
            action.run();
        } else {
            sHandler.post(action);
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
