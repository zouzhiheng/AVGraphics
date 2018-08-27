package com.steven.avgraphics.activity.gles;

import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.opengl.Matrix;
import android.os.Bundle;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import com.steven.avgraphics.BaseActivity;
import com.steven.avgraphics.R;
import com.steven.avgraphics.util.Utils;

import java.nio.ByteBuffer;

public class TextureImageActivity extends BaseActivity {

    private Bitmap mBitmap;
    private byte[] mPixel;
    private float[] mMvpMatrix = new float[16];

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_texture_image);
        init();
    }

    private void init() {
        initData();
        initSurfaceView();
        setListener();
    }

    private void initData() {
        Matrix.setIdentityM(mMvpMatrix, 0);
        BitmapFactory.Options options = new BitmapFactory.Options();
        options.inPreferredConfig = Bitmap.Config.ARGB_8888;
        mBitmap = BitmapFactory.decodeResource(getResources(), R.drawable.tex_cat, options);
        int byteCount = mBitmap.getByteCount();
        ByteBuffer buffer = ByteBuffer.allocate(byteCount);
        mBitmap.copyPixelsToBuffer(buffer);
        mPixel = buffer.array();
    }

    private void initSurfaceView() {
        SurfaceView surfaceView = findViewById(R.id.tex_sv_window);
        surfaceView.getLayoutParams().width = Utils.getScreenWidth();
        surfaceView.getLayoutParams().height = Utils.getScreenWidth();
        surfaceView.getHolder().addCallback(new SurfaceCallback());
    }

    private void setListener() {
        findViewById(R.id.tex_btn_origin).setOnClickListener(v -> redraw(Filter.NONE));
        findViewById(R.id.tex_btn_gray).setOnClickListener(v -> redraw(Filter.GRAY));
        findViewById(R.id.tex_btn_blur).setOnClickListener(v -> redraw(Filter.BLUR));
        findViewById(R.id.tex_btn_cool).setOnClickListener(v -> redraw(Filter.COOL));
        findViewById(R.id.tex_btn_warm).setOnClickListener(v -> redraw(Filter.WARM));
    }

    private void redraw(Filter filter) {
        _draw(mMvpMatrix, filter.mType, filter.mData);
    }

    private class SurfaceCallback implements SurfaceHolder.Callback {

        @Override
        public void surfaceCreated(SurfaceHolder holder) {
            AssetManager assetManager = getAssets();
            _init(holder.getSurface(), mBitmap.getWidth(), mBitmap.getHeight(), mPixel,
                    mPixel.length, assetManager);
        }

        @Override
        public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
            _resize(width, height);
            setMatrix(width, height);
            _draw(mMvpMatrix, Filter.NONE.mType, Filter.NONE.mData);
        }

        @Override
        public void surfaceDestroyed(SurfaceHolder holder) {
            _release();
        }

        private void setMatrix(int width, int height) {
            float[] viewMatrix = new float[16];
            float[] projectMatrix = new float[16];
            int bw = mBitmap.getWidth();
            int bh = mBitmap.getHeight();
            float bitmapRatio = 1.0f * bw / bh;
            float surfaceRatio = 1.0f * width / height;
            if (width > height) {
                if (bitmapRatio > surfaceRatio) {
                    Matrix.orthoM(projectMatrix, 0, -surfaceRatio * bitmapRatio,
                            surfaceRatio * bitmapRatio, -1, 1, 3, 5);
                } else {
                    Matrix.orthoM(projectMatrix, 0, -surfaceRatio / bitmapRatio,
                            surfaceRatio / bitmapRatio, -1, 1, 3, 5);
                }
            } else {
                if (bitmapRatio > surfaceRatio) {
                    Matrix.orthoM(projectMatrix, 0, -1, 1,
                            -1 / surfaceRatio * bitmapRatio,
                            1 / surfaceRatio * bitmapRatio, 3, 5);
                } else {
                    Matrix.orthoM(projectMatrix, 0, -1, 1,
                            -bitmapRatio / surfaceRatio,
                            bitmapRatio / surfaceRatio, 3, 5);
                }
            }
            Matrix.setLookAtM(viewMatrix, 0, 0, 0, 5.0f, 0f,
                    0f, 0f, 0f, 1.0f, 0.0f);
            Matrix.multiplyMM(mMvpMatrix, 0, projectMatrix, 0, viewMatrix, 0);
        }

    }

    public enum Filter {
        NONE(0, new float[]{0.0f, 0.0f, 0.0f}),
        GRAY(1, new float[]{0.299f, 0.587f, 0.114f}),
        COOL(2, new float[]{0.0f, 0.0f, 0.1f}),
        WARM(2, new float[]{0.1f, 0.1f, 0.0f}),
        BLUR(3, new float[]{0.006f, 0.004f, 0.002f});

        private int mType;
        private float[] mData;

        Filter(int mType, float[] data) {
            this.mType = mType;
            this.mData = data;
        }
    }

    private static native void _init(Surface surface, int texWidth, int texHeight, byte[] pixel,
                                     int pixelDataLen, AssetManager assetManager);

    private static native void _resize(int width, int height);

    private static native void _draw(float[] matrix, int filterType, float[] filterColor);

    private static native void _release();

}
