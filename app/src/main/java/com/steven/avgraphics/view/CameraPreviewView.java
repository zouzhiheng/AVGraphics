package com.steven.avgraphics.view;


import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Color;
import android.hardware.Camera;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.widget.FrameLayout;
import android.widget.ImageView;

import com.steven.avgraphics.R;
import com.steven.avgraphics.util.CameraHelper;
import com.steven.avgraphics.util.ToastHelper;

import java.io.IOException;

public class CameraPreviewView extends FrameLayout {

    private static final String TAG = "CameraPreviewView";

    private ImageView mIvFocus;
    private ImageView mIvIndicator;
    private Animation mFocusAnimation;
    private Animation mIndicatorAnimation;

    private int mFocusWidth = LayoutParams.WRAP_CONTENT;
    private int mFocusHeight = LayoutParams.WRAP_CONTENT;
    private int mIndicatorWidth = LayoutParams.WRAP_CONTENT;
    private int mIndicatorHeight = LayoutParams.WRAP_CONTENT;

    private Camera mCamera;
    private CameraSurfaceView mSurfaceView;

    public CameraPreviewView(@NonNull Context context) {
        super(context);
        init(context, null);
    }

    public CameraPreviewView(@NonNull Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
        init(context, attrs);
    }

    private void init(Context context, AttributeSet attrs) {
        setBackgroundColor(Color.BLACK);

        if (attrs != null) {
            TypedArray array = context.obtainStyledAttributes(attrs, R.styleable.CameraPreviewView);
            mFocusWidth = array.getDimensionPixelSize(R.styleable.CameraPreviewView_focus_width,
                    LayoutParams.WRAP_CONTENT);
            mFocusHeight = array.getDimensionPixelSize(R.styleable.CameraPreviewView_focus_height,
                    LayoutParams.WRAP_CONTENT);
            mIndicatorWidth = array.getDimensionPixelSize(R.styleable.CameraPreviewView_indicator_width,
                    LayoutParams.WRAP_CONTENT);
            mIndicatorHeight = array.getDimensionPixelSize(R.styleable.CameraPreviewView_indicator_height,
                    LayoutParams.WRAP_CONTENT);
            array.recycle();
        }

        initAnimation(context);
        addView(context);
    }

    private void addView(Context context) {
        addView(new View(context), new LayoutParams(LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));

        mIvFocus = new ImageView(context);
        mIvFocus.setImageResource(R.drawable.cpv_focus);
        mIvIndicator = new ImageView(context);
        mIvIndicator.setImageResource(R.drawable.cpv_indicator);

        addView(mIvFocus, new LayoutParams(mFocusWidth, mFocusHeight, Gravity.CENTER));
        addView(mIvIndicator, new LayoutParams(mIndicatorWidth, mIndicatorHeight, Gravity.CENTER));

        mSurfaceView = new CameraSurfaceView(context);
        addView(mSurfaceView, 0, new LayoutParams(LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
    }

    private void initAnimation(Context context) {
        mFocusAnimation = AnimationUtils.loadAnimation(context, R.anim.cpv_focus);
        mIndicatorAnimation = AnimationUtils.loadAnimation(context, R.anim.cpv_indicator);
        mFocusAnimation.setAnimationListener((AnimationEndListener) animation ->
                mIvFocus.setVisibility(INVISIBLE));
        mIndicatorAnimation.setAnimationListener((AnimationEndListener) animation ->
                mIvIndicator.setVisibility(INVISIBLE));
    }

    public Camera getCamera() {
        return mCamera;
    }

    private class CameraSurfaceView extends SurfaceView implements SurfaceHolder.Callback {

        private float mClickX;
        private float mClickY;

        public CameraSurfaceView(Context context) {
            super(context);
            getHolder().addCallback(this);
        }

        public CameraSurfaceView(Context context, AttributeSet attrs) {
            super(context, attrs);
            getHolder().addCallback(this);
        }

        @Override
        public void surfaceCreated(SurfaceHolder holder) {

        }

        @Override
        public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
            openCamera(holder, width, height);
        }

        @Override
        public void surfaceDestroyed(SurfaceHolder holder) {
            mFocusAnimation.cancel();
            mIndicatorAnimation.cancel();
            releaseCamera();
        }

        private void openCamera(SurfaceHolder holder, int width, int height) {
            if (mCamera != null) {
                return;
            }
            mCamera = CameraHelper.openCamera();
            if (mCamera == null) {
                return;
            }

            Camera.Parameters parameters = mCamera.getParameters();
            Camera.Size size = CameraHelper.chooseCameraSize(parameters.getSupportedPreviewSizes(), width, height);
            parameters.setPreviewSize(size.width, size.height);
            mCamera.setParameters(parameters);
            mCamera.setDisplayOrientation(90);
            try {
                mCamera.setPreviewDisplay(holder);
                mCamera.startPreview();
                mIndicatorAnimation.cancel();
                mIvIndicator.startAnimation(mIndicatorAnimation);
                cameraFocus(CameraPreviewView.this.getWidth() / 2.0f, CameraPreviewView.this.getHeight() / 2.0f);
            } catch (IOException e) {
                Log.e(TAG, "openCamera preview failed: " + e.getLocalizedMessage());
                ToastHelper.show("相机预览开启失败！");
                releaseCamera();
            }
            Log.i(TAG, "--- openCamera");
        }

        private void cameraFocus(final float x, final float y) {
            mCamera.cancelAutoFocus();
            CameraHelper.setFocusMode(mCamera, Camera.Parameters.FOCUS_MODE_AUTO);

            mCamera.autoFocus((success, camera) ->
                    CameraHelper.setFocusMode(mCamera, Camera.Parameters.FOCUS_MODE_CONTINUOUS_VIDEO));

            mFocusAnimation.cancel();
            mIvFocus.clearAnimation();
            int left = (int) (x - mIvFocus.getWidth() / 2f);
            int top = (int) (y - mIvFocus.getHeight() / 2f);
            int right = left + mIvFocus.getWidth();
            int bottom = top + mIvFocus.getHeight();
            mIvFocus.layout(left, top, right, bottom);
            mIvFocus.setVisibility(VISIBLE);
            mIvFocus.startAnimation(mFocusAnimation);
        }

        private void releaseCamera() {
            if (mCamera != null) {
                mCamera.setPreviewCallback(null);
                mCamera.stopPreview();
                mCamera.release();
                mCamera = null;
            }
        }

        @Override
        public boolean onTouchEvent(MotionEvent event) {
            super.onTouchEvent(event);
            if (event.getAction() == MotionEvent.ACTION_DOWN) {
                mClickX = event.getX();
                mClickY = event.getY();
                performClick();
                return true;
            }
            return false;
        }

        @Override
        public boolean performClick() {
            super.performClick();
            cameraFocus(mClickX, mClickY);
            return true;
        }

    }

    private interface AnimationEndListener extends Animation.AnimationListener {
        @Override
        default void onAnimationStart(Animation animation) {

        }

        @Override
        default void onAnimationRepeat(Animation animation) {

        }
    }


}
