package com.steven.avgraphics.util;


import android.app.Activity;
import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.widget.Toast;

import com.steven.avgraphics.BaseApplication;

@SuppressWarnings("ALL")
public class ToastHelper {

    public static void show(String msg) {
        Toast.makeText(BaseApplication.getContext(), msg, Toast.LENGTH_SHORT).show();
    }

    public static void show(Context context, String msg) {
        Toast.makeText(context, msg, Toast.LENGTH_SHORT).show();
    }

    public static void showLong(String msg) {
        Toast.makeText(BaseApplication.getContext(), msg, Toast.LENGTH_LONG).show();
    }

    public static void showLong(Context context, String msg) {
        Toast.makeText(context, msg, Toast.LENGTH_LONG).show();
    }

    public static void showOnUiThread(final Activity activity, final String msg) {
        activity.runOnUiThread(() -> Toast.makeText(activity, msg, Toast.LENGTH_SHORT).show());
    }

    public static void showOnUiThread(final String msg) {
        if (Looper.myLooper() != Looper.getMainLooper()) {
            new Handler(Looper.getMainLooper()).post(() ->
                    Toast.makeText(BaseApplication.getContext(), msg, Toast.LENGTH_SHORT).show());
        } else {
            show(msg);
        }
    }

}
