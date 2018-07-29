package com.steven.avgraphics.view;


import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import com.steven.avgraphics.R;


public class DrawableSurfaceView extends SurfaceView implements SurfaceHolder.Callback {

    private Bitmap mBitmap;

    public DrawableSurfaceView(Context context) {
        super(context);
        init(context, null);
    }

    public DrawableSurfaceView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(context, attrs);
    }

    private void init(Context context, AttributeSet attrs) {
        Drawable drawable;
        if (attrs != null) {
            TypedArray array = context.obtainStyledAttributes(attrs, R.styleable.DrawableSurfaceView);
            drawable = array.getDrawable(R.styleable.DrawableSurfaceView_src);
            array.recycle();
        } else {
            drawable = context.getResources().getDrawable(R.mipmap.draw_image_surface);
        }
        if (drawable == null) {
            throw new RuntimeException("DrawableSurfaceView get null drawable");
        }
        getHolder().addCallback(this);
        mBitmap = ((BitmapDrawable) drawable).getBitmap();
        drawable.setCallback(this);
        drawable.setLevel(0);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        canvas.drawColor(Color.BLACK);
        canvas.drawBitmap(mBitmap, 0, 0, null);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        Canvas canvas = null;
        try {
            canvas = holder.lockCanvas();
            draw(canvas);
        } finally {
            if (canvas != null) {
                holder.unlockCanvasAndPost(canvas);
            }
        }
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }

}
