package com.steven.avgraphics.view;


import android.app.Activity;
import android.content.Context;
import android.content.res.AssetManager;
import android.content.res.TypedArray;
import android.graphics.Color;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.Surface;
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

import java.io.IOException;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;

public class CameraPreviewView extends FrameLayout {

    private static final String TAG = "CameraPreviewView";
    private static final float[] ASPECT_RATIO_ARRAY = {9.0f / 16, 3.0f / 4};

    private static final float MIN_BEAUTY = 0;
    private static final float MAX_BEAUTY = 1;
    private static final float MIN_BRIGHT = 0;
    private static final float MAX_BRIGHT = 1;
    private static final float MIN_SATURATE = 0;
    private static final float MAX_SATURATE = 1;

    private ImageView mIvFocus;
    private ImageView mIvIndicator;
    private Animation mFocusAnimation;
    private Animation mIndicatorAnimation;

    private int mFocusWidth = LayoutParams.WRAP_CONTENT;
    private int mFocusHeight = LayoutParams.WRAP_CONTENT;
    private int mIndicatorWidth = LayoutParams.WRAP_CONTENT;
    private int mIndicatorHeight = LayoutParams.WRAP_CONTENT;

    private Camera mCamera;
    private int mCameraId = Camera.CameraInfo.CAMERA_FACING_BACK;
    private CameraSurfaceView mSurfaceView;
    private PreviewCallback mPreviewCallback;
    public float mAspectRatio = ASPECT_RATIO_ARRAY[0];

    private boolean mIsBeautyOpen = false;
    private boolean mIsBeautyOnFront = false;
    private float mBeautyLevel = MIN_BEAUTY;
    private float mSaturateLevel = MIN_SATURATE;
    private float mBrightLevel = MIN_BRIGHT;
    private boolean mIsRecording = false;

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
            mIsBeautyOnFront = array.getBoolean(R.styleable.CameraPreviewView_beauty_on_front, false);
            mCameraId = array.getInt(R.styleable.CameraPreviewView_facing, Camera.CameraInfo.CAMERA_FACING_BACK);
            array.recycle();
        }

        mIsBeautyOpen = mIsBeautyOnFront && !CameraHelper.isFacingBack(mCameraId);
        initAnimation(context);
        addView(context);
    }

    private void initAnimation(Context context) {
        mFocusAnimation = AnimationUtils.loadAnimation(context, R.anim.cpv_focus);
        mIndicatorAnimation = AnimationUtils.loadAnimation(context, R.anim.cpv_indicator);
        mFocusAnimation.setAnimationListener((AnimationEndListener) animation ->
                mIvFocus.setVisibility(INVISIBLE));
        mIndicatorAnimation.setAnimationListener((AnimationEndListener) animation ->
                mIvIndicator.setVisibility(INVISIBLE));
    }

    private void addView(Context context) {
        // 添加一个 View 占满屏幕，否则 mIvFocus 在屏幕下方显示时会被切掉一部分，原因未知
        addView(new View(context), new LayoutParams(LayoutParams.MATCH_PARENT,
                LayoutParams.MATCH_PARENT));

        mIvFocus = new ImageView(context);
        mIvFocus.setImageResource(R.drawable.cpv_focus);
        mIvIndicator = new ImageView(context);
        mIvIndicator.setImageResource(R.drawable.cpv_indicator);

        addView(mIvFocus, new LayoutParams(mFocusWidth, mFocusHeight, Gravity.CENTER));
        addView(mIvIndicator, new LayoutParams(mIndicatorWidth, mIndicatorHeight, Gravity.CENTER));

        mSurfaceView = new CameraSurfaceView(context);
        addView(mSurfaceView, 0, new LayoutParams(LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT));
    }

    public void setPreviewCallback(PreviewCallback previewCallback) {
        mPreviewCallback = previewCallback;
    }

    // 磨皮
    public void setBeautyLevel(float beautyLevel) {
        if (beautyLevel < MIN_BEAUTY || beautyLevel > MAX_BEAUTY) {
            Log.e(TAG, "setBeautyLevel invalid argument");
            return;
        }
        mBeautyLevel = MIN_BEAUTY + (MAX_BEAUTY - MIN_BEAUTY) * beautyLevel;
    }

    // 红润(饱和度)
    public void setSaturateLevel(float saturateLevel) {
        if (saturateLevel < MIN_SATURATE || saturateLevel > MAX_SATURATE) {
            Log.e(TAG, "setSaturateLevel invalid argument");
            return;
        }
        mSaturateLevel = MIN_SATURATE + (MAX_SATURATE - MIN_SATURATE) * saturateLevel;
    }

    // 美白(亮度)
    public void setBrightLevel(float brightLevel) {
        if (brightLevel < MIN_BRIGHT || brightLevel > MAX_BRIGHT) {
            Log.e(TAG, "setBrightLevel invalid argument");
            return;
        }
        mBrightLevel = MIN_BRIGHT + (MAX_BRIGHT - MIN_BRIGHT) * brightLevel;
    }

    public void setRecording(boolean recording) {
        mIsRecording = recording;
    }

    public boolean isFacingBack() {
        return CameraHelper.isFacingBack(mCameraId);
    }

    public boolean isBeautyOpen() {
        return mIsBeautyOpen;
    }

    // 前置摄像头启用了美颜功能，传给 OpenGL 的宽高为 surface 的宽高
    public int getImageWidth() {
        return mIsBeautyOpen
                ? mSurfaceView.getSurfaceWidth()
                : mCamera.getParameters().getPreviewSize().width;
    }

    public int getImageHeight() {
        return mIsBeautyOpen
                ? mSurfaceView.getSurfaceHeight()
                : mCamera.getParameters().getPreviewSize().height;
    }

    public void switchCamera() {
        if (mSurfaceView != null) {
            removeView(mSurfaceView);
        }
        mCameraId = 1 - mCameraId;
        mIsBeautyOpen = !CameraHelper.isFacingBack(mCameraId) && mIsBeautyOnFront;
        mSurfaceView = new CameraSurfaceView(getContext());
        addView(mSurfaceView, 0, new LayoutParams(LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT));
    }

    public void release() {
        mSurfaceView.releaseCamera();
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        // 屏幕空间足够就显示 16:9，否则显示 4:3
        int width = MeasureSpec.getSize(widthMeasureSpec);
        int maxHeight = MeasureSpec.getSize(heightMeasureSpec);
        int height = (int) (width / ASPECT_RATIO_ARRAY[0]);
        for (float ratio : ASPECT_RATIO_ARRAY) {
            height = (int) (width / ratio);
            if (height <= maxHeight) {
                mAspectRatio = ratio;
                break;
            }
        }
        int wms = MeasureSpec.makeMeasureSpec(width, MeasureSpec.EXACTLY);
        int hms = MeasureSpec.makeMeasureSpec(height, MeasureSpec.EXACTLY);
        super.onMeasure(wms, hms);
    }

    @Override
    protected void onDetachedFromWindow() {
        super.onDetachedFromWindow();
        mFocusAnimation.cancel();
        mIndicatorAnimation.cancel();
    }

    private class CameraSurfaceView extends SurfaceView implements SurfaceHolder.Callback {

        private ExecutorService mExecutor = Executors.newSingleThreadExecutor();
        private SurfaceTexture mSurfaceTexture;

        private float[] mMatrix = new float[16];
        private float mClickX;
        private float mClickY;

        private int mSurfaceWidth;
        private int mSurfaceHeight;

        public CameraSurfaceView(Context context) {
            super(context);
            getHolder().addCallback(this);
        }

        public CameraSurfaceView(Context context, AttributeSet attrs) {
            super(context, attrs);
            getHolder().addCallback(this);
        }

        public int getSurfaceWidth() {
            return mSurfaceWidth;
        }

        public int getSurfaceHeight() {
            return mSurfaceHeight;
        }

        @Override
        public void surfaceCreated(SurfaceHolder holder) {

        }

        @Override
        public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
            openCamera(holder, width, height);
            mSurfaceWidth = width;
            mSurfaceHeight = height;
        }

        @Override
        public void surfaceDestroyed(SurfaceHolder holder) {
            if (mPreviewCallback != null) {
                mPreviewCallback.onPreviewStopped();
            }
            releaseCamera();
            if (mIsBeautyOpen) {
                releaseOpenGL();
            }
        }

        @SuppressWarnings("SuspiciousNameCombination")
        private void openCamera(SurfaceHolder holder, int width, int height) {
            if (mCamera != null) {
                return;
            }
            mCamera = CameraHelper.openCamera(mCameraId);
            if (mCamera == null) {
                return;
            }

            CameraHelper.setOptimalSize(mCamera, mAspectRatio, height, width);
            CameraHelper.setDisplayOritation((Activity) getContext(), mCamera, mCameraId);
            startPreview(holder, width, height);
        }

        private void startPreview(SurfaceHolder holder, int width, int height) {
            if (mIsBeautyOpen) {
                try {
                    mSurfaceTexture = initOpenGL(holder.getSurface(), width, height);
                } catch (ExecutionException | InterruptedException e) {
                    releaseOpenGL();
                    mIsBeautyOpen = false;
                    Log.e(TAG, "startPreview draw with opengl failed: " + e.getMessage());
                }
            }

            try {
                if (mIsBeautyOpen) {
                    mCamera.setPreviewTexture(mSurfaceTexture);
                } else {
                    mCamera.setPreviewDisplay(holder);
                }
                doStartPreview();
            } catch (IOException e) {
                Log.e(TAG, "startPreview open camera failed: " + e.getMessage());
                releaseCamera();
                if (mPreviewCallback != null) {
                    mPreviewCallback.onPreviewFailed();
                }
            }
        }

        private SurfaceTexture initOpenGL(Surface surface, int width, int height)
                throws ExecutionException, InterruptedException {
            Future<SurfaceTexture> future = mExecutor.submit(() -> {
                AssetManager manager = getContext().getAssets();
                int textureId = _init(surface, width, height, manager);
                if (textureId < 0) {
                    Log.e(TAG, "surfaceCreated init OpenGL ES failed!");
                    mIsBeautyOpen = false;
                    return null;
                }
                SurfaceTexture surfaceTexture = new SurfaceTexture(textureId);
                surfaceTexture.setOnFrameAvailableListener(surfaceTexture1 -> drawOpenGL());
                return surfaceTexture;
            });
            return future.get();
        }

        private void drawOpenGL() {
            mExecutor.execute(() -> {
                if (mSurfaceTexture != null) {
                    mSurfaceTexture.updateTexImage(); // 必须运行在 OpenGL 线程环境中
                    mSurfaceTexture.getTransformMatrix(mMatrix);
                    _draw(mMatrix, mBeautyLevel, mSaturateLevel, mBrightLevel, mIsRecording);
                }
            });
        }

        private void releaseOpenGL() {
            mExecutor.execute(() -> {
                if (mSurfaceTexture != null) {
                    mSurfaceTexture.release();
                    mSurfaceTexture = null;
                }
                _stop();
            });
        }

        private void doStartPreview() throws IOException {
            mCamera.startPreview();
            mIndicatorAnimation.cancel();
            mIvIndicator.startAnimation(mIndicatorAnimation);
            cameraFocus(CameraPreviewView.this.getWidth() / 2.0f, CameraPreviewView.this.getHeight() / 2.0f);
            if (mPreviewCallback != null) {
                mPreviewCallback.onPreviewStarted(mCamera);
            }
        }

        private void cameraFocus(final float x, final float y) {
            mCamera.cancelAutoFocus();
            CameraHelper.setFocusMode(mCamera, Camera.Parameters.FOCUS_MODE_CONTINUOUS_VIDEO);

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

    public interface PreviewCallback {
        void onPreviewStarted(Camera camera);

        void onPreviewStopped();

        void onPreviewFailed();
    }

    private interface AnimationEndListener extends Animation.AnimationListener {
        @Override
        default void onAnimationStart(Animation animation) {

        }

        @Override
        default void onAnimationRepeat(Animation animation) {

        }
    }

    private static native int _init(Surface surface, int width, int height, AssetManager manager);

    private static native void _draw(float[] matrix, float beauty, float saturate, float bright,
                                     boolean recording);

    private static native void _stop();


}
