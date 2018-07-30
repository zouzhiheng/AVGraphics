package com.steven.avgraphics.activity.gles;

import android.content.Context;
import android.content.Intent;
import android.support.annotation.IntDef;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import com.steven.avgraphics.R;
import com.steven.avgraphics.util.Utils;

public class ShapeActivity extends AppCompatActivity {

    private static final String KEY_SHAPE = "shape";

    public static final int SHAPE_TRIANGLE = 1;
    public static final int SHAPE_CIRCLE = 2;
    public static final int SHAPE_SQUARE = 3;

    public static void start(Context context, @Shape int shape) {
        Intent intent = new Intent(context, ShapeActivity.class);
        intent.putExtra(KEY_SHAPE, shape);
        context.startActivity(intent);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_shape);
        init();
    }

    private void init() {
        int shape = getIntent().getIntExtra(KEY_SHAPE, SHAPE_TRIANGLE);
        SurfaceView surfaceView = findViewById(R.id.shape_sv_window);
        surfaceView.getLayoutParams().width = Utils.getScreenWidth();
        surfaceView.getLayoutParams().height = Utils.getScreenWidth();
        surfaceView.getHolder().addCallback(new SurfaceCallback(shape));
    }

    private class SurfaceCallback implements SurfaceHolder.Callback {

        private int mShape;

        private SurfaceCallback(int shape) {
            mShape = shape;
        }

        @Override
        public void surfaceCreated(SurfaceHolder holder) {

        }

        @Override
        public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
            _init(holder.getSurface(), width, height, mShape);
            _draw();
        }

        @Override
        public void surfaceDestroyed(SurfaceHolder holder) {
            _release();
        }

    }

    private static native void _init(Surface surface, int width, int height, int shape);

    private static native void _draw();

    private static native void _release();

    @IntDef({SHAPE_TRIANGLE, SHAPE_CIRCLE, SHAPE_SQUARE})
    public @interface Shape {

    }

}
