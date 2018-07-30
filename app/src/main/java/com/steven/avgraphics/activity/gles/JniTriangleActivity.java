package com.steven.avgraphics.activity.gles;

import android.opengl.GLSurfaceView;
import android.os.Bundle;

import com.steven.avgraphics.BaseActivity;
import com.steven.avgraphics.R;
import com.steven.avgraphics.util.Utils;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class JniTriangleActivity extends BaseActivity {

    private GLSurfaceView mGLSurfaceView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_jni_triangle);
        init();
    }

    private void init() {
        mGLSurfaceView = findViewById(R.id.jnitri_glsv_window);
        mGLSurfaceView.getLayoutParams().width = Utils.getScreenWidth();
        mGLSurfaceView.getLayoutParams().height = Utils.getScreenWidth();

        mGLSurfaceView.setEGLConfigChooser(8, 8, 8, 0,
                0, 0);
        mGLSurfaceView.setEGLContextClientVersion(3);
        mGLSurfaceView.setRenderer(new TriangleRenderer());
        mGLSurfaceView.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
    }

    @Override
    protected void onPause() {
        super.onPause();
        mGLSurfaceView.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        mGLSurfaceView.onResume();
    }

    private class TriangleRenderer implements GLSurfaceView.Renderer {

        @Override
        public void onSurfaceCreated(GL10 gl, EGLConfig config) {
            _init();
        }

        @Override
        public void onSurfaceChanged(GL10 gl, int width, int height) {
            _resize(width, height);
        }

        @Override
        public void onDrawFrame(GL10 gl) {
            _draw();
        }

    }

    private static native void _init();

    private static native void _resize(int width, int height);

    private static native void _draw();

}
