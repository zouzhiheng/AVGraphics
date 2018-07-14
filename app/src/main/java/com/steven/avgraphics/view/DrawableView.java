package com.steven.avgraphics.view;


import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Rect;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.support.annotation.Nullable;
import android.util.AttributeSet;
import android.view.View;

import com.steven.avgraphics.R;


public class DrawableView extends View {

    private static final String TAG = "DrawableView";

    private Bitmap mBitmap;
    private Rect mRectSrc;
    private Rect mRectDst;

    public DrawableView(Context context) {
        super(context);
        init(context, null);
    }

    public DrawableView(Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
        init(context, attrs);
    }

    private void init(Context context, AttributeSet attrs) {
        Drawable drawable;
        if (attrs != null) {
            TypedArray array = context.obtainStyledAttributes(attrs, R.styleable.DrawableView);
            drawable = array.getDrawable(R.styleable.DrawableView_src);
            array.recycle();
        } else {
            drawable = context.getResources().getDrawable(R.mipmap.draw_image_custom);
        }
        if (drawable == null) {
            throw new RuntimeException("DrawableSurfaceView get null drawable");
        }
        mBitmap = ((BitmapDrawable) drawable).getBitmap();
        drawable.setCallback(this);
        drawable.setLevel(0);
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        int widthMode = MeasureSpec.getMode(widthMeasureSpec);
        int heightMode = MeasureSpec.getMode(heightMeasureSpec);
        int widthSize = MeasureSpec.getSize(widthMeasureSpec);
        int heightSize = MeasureSpec.getSize(heightMeasureSpec);

        int width;
        int height;

        if (widthMode == MeasureSpec.EXACTLY) {
            width = widthSize;
        } else if (widthMode == MeasureSpec.AT_MOST) {
            width = Math.min(mBitmap.getWidth(), widthSize);
        } else {
            width = mBitmap.getWidth();
        }

        if (heightMode == MeasureSpec.EXACTLY) {
            height = heightSize;
        } else if (heightMode == MeasureSpec.AT_MOST) {
            height = Math.min(mBitmap.getHeight(), heightSize);
        } else {
            height = mBitmap.getHeight();
        }

        setMeasuredDimension(width, height);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        final int paddingLeft = getPaddingLeft();
        final int paddingTop = getPaddingTop();
        final int paddingRight = getPaddingRight();
        final int paddingBottom = getPaddingBottom();
        // 支持 padding
        int width = getWidth() - paddingRight;
        int height = getHeight() - paddingBottom;
        canvas.drawColor(Color.BLACK);
        if (mRectSrc == null) {
            mRectSrc = new Rect(0, 0, mBitmap.getWidth(), mBitmap.getHeight());
        }
        if (mRectDst == null || mRectDst.left != paddingLeft || mRectDst.top != paddingTop
                || mRectDst.right != width || mRectDst.bottom != height) {
            mRectDst = new Rect(paddingLeft, paddingTop, width, height);
        }
        canvas.drawBitmap(mBitmap, mRectSrc, mRectDst, null);
    }

}
