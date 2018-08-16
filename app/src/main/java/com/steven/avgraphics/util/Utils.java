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

    private static final String HW_RECORD_OUTPUT = APP_DIR + "/hwrecord.mp4";
    private static final String HW_DECODE_YUV_OUTPUT = APP_DIR + "/hwdecode.yuv";
    private static final String HW_DECODE_PCM_OUTPUT = APP_DIR + "/hwdecode.pcm";
    private static final String HW_TRANSCODE_OUTPUT = APP_DIR + "/hwtranscode.mp4";

    private static final String OPENSL_OUTPUT = APP_DIR + "/opensl.pcm";

    private static final String FF_RECORD_OUTPUT = APP_DIR + "/ffrecord.mp4";
    private static final String FF_TRANSCODE_OUTPUT = APP_DIR + "/fftranscode.mp4";
    private static final String FF_DECODE_YUV_OUTPUT = APP_DIR + "/ffdecode.yuv";
    private static final String FF_DECODE_PCM_OUTPUT = APP_DIR + "/ffdecode.pcm";

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
        return HW_RECORD_OUTPUT;
    }

    public static String getHWDecodeYuvOutput() {
        return HW_DECODE_YUV_OUTPUT;
    }

    public static String getHWDecodePcmOutput() {
        return HW_DECODE_PCM_OUTPUT;
    }

    public static String getHWTranscodeOutput() {
        return HW_TRANSCODE_OUTPUT;
    }

    public static String getFFDecodeYuvOutput() {
        return FF_DECODE_YUV_OUTPUT;
    }

    public static String getFFDecodePcmOutput() {
        return FF_DECODE_PCM_OUTPUT;
    }

    public static String getFFRecordOutput() {
        return FF_RECORD_OUTPUT;
    }

    public static String getFFTranscodeOutput() {
        return FF_TRANSCODE_OUTPUT;
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
