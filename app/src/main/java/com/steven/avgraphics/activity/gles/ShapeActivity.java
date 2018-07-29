package com.steven.avgraphics.activity.gles;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import com.steven.avgraphics.R;
import com.steven.avgraphics.util.Utils;

public class ShapeActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_shape);
        init();
    }

    private void init() {
        SurfaceView surfaceView = findViewById(R.id.shape_sv_window);
        surfaceView.getLayoutParams().width = Utils.getScreenWidth();
        surfaceView.getLayoutParams().height = Utils.getScreenWidth();
        surfaceView.getHolder().addCallback(new SurfaceCallback());
    }

    private class SurfaceCallback implements SurfaceHolder.Callback {

        @Override
        public void surfaceCreated(SurfaceHolder holder) {

        }

        @Override
        public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
            _init(holder.getSurface(), width, height);
            _draw();
        }

        @Override
        public void surfaceDestroyed(SurfaceHolder holder) {
            _release();
        }

    }

    private static native void _init(Surface surface, int width, int height);

    private static native void _draw();

    private static native void _release();


}
