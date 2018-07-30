package com.steven.avgraphics.activity.gles;

import android.opengl.GLES30;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.util.Log;

import com.steven.avgraphics.BaseActivity;
import com.steven.avgraphics.R;
import com.steven.avgraphics.util.Utils;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.nio.IntBuffer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class JavaTriangleActivity extends BaseActivity {

    private static final String VERTEX_SHADER = "" +
            "#version 300 es\n" +
            "layout(location=0) in vec4 aColor;\n" +
            "layout(location=1) in vec4 aPosition;\n" +
            "out vec4 vColor;\n" +
            "void main() {\n" +
            "   vColor = aColor;\n" +
            "   gl_Position = aPosition;\n" +
            "}\n";

    private static final String FRAGMENT_SHADER = "" +
            "#version 300 es\n" +
            // 高精度精确，低精度效率高，这里全局指定了默认精度，可单独设置某个变量的精度
            "precision mediump float;\n" +
            "in vec4 vColor;\n" +
            "out vec4 fragColor;\n" +
            "void main() {\n" +
            "   fragColor = vColor;\n" +
            "}\n";

    // 每个顶点使用 (x, y, z) 表示，因此共三个顶点坐标点，描述了一个三角形
    private static final float VERTEX_ARR[] = {
            -0.5f, 0.0f, 0.0f,
            0.5f, 0.0f, 0.0f,
            0.0f, 0.5f, 0.0f
    };
    private static FloatBuffer sVertices;

    private static final float COLORS[] = {
            0.0f, 1.0f, 1.0f, 1.0f
    };

    private static final int ATTRIB_COLOR = 0; // 对应 location = 0
    private static final int ATTRIB_POSITION = 1; // 对应 location = 1
    private static final int VERTEX_COUNT = 3;

    private GLSurfaceView mGLSurfaceView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_java_triangle);
        init();
    }

    private void init() {
        initGLSurfaceView();
        initVerties();
    }

    private void initGLSurfaceView() {
        mGLSurfaceView = findViewById(R.id.jvtri_glsv_window);
        // 宽高 1:1，避免出现坐标系 x、y 轴单位长度不同，而导致图形拉伸的现象
        mGLSurfaceView.getLayoutParams().width = Utils.getScreenWidth();
        mGLSurfaceView.getLayoutParams().height = Utils.getScreenWidth();

        // 设置 EGL 环境
        mGLSurfaceView.setEGLConfigChooser(8, 8, 8, 0,
                0, 0);
        mGLSurfaceView.setEGLContextClientVersion(3);
        mGLSurfaceView.setRenderer(new TriangleRenderer());
        mGLSurfaceView.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
    }

    private void initVerties() {
        ByteBuffer buffer = ByteBuffer.allocateDirect(VERTEX_ARR.length * 4);
        buffer.order(ByteOrder.nativeOrder());
        sVertices = buffer.asFloatBuffer();
        sVertices.put(VERTEX_ARR);
        sVertices.position(0);
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

        private int mProgram;
        private int mWidth;
        private int mHeight;

        @Override
        public void onSurfaceCreated(GL10 gl, EGLConfig config) {
            mProgram = loadProgram();
            if (mProgram == 0) {
                finishWithToast(R.string.jvtri_msg_create_program_failed);
                return;
            }
            GLES30.glClearColor(0.66f, 0.66f, 0.66f, 1);
        }

        @Override
        public void onSurfaceChanged(GL10 gl, int width, int height) {
            mWidth = width;
            mHeight = height;
        }

        @Override
        public void onDrawFrame(GL10 gl) {
            GLES30.glViewport(0, 0, mWidth, mHeight);
            GLES30.glClear(GLES30.GL_COLOR_BUFFER_BIT);
            GLES30.glUseProgram(mProgram);

            GLES30.glVertexAttrib4fv(ATTRIB_COLOR, COLORS, 0);
            GLES30.glVertexAttribPointer(ATTRIB_POSITION, VERTEX_COUNT, GLES30.GL_FLOAT,
                    false, 0, sVertices);
            GLES30.glEnableVertexAttribArray(ATTRIB_POSITION);
            GLES30.glDrawArrays(GLES30.GL_TRIANGLES, 0, VERTEX_COUNT);
            GLES30.glDisableVertexAttribArray(ATTRIB_POSITION);
        }

        private int loadProgram() {
            int program = GLES30.glCreateProgram();
            if (program <= 0) {
                Log.e(TAG, "onSurfaceCreated create program failed: " + GLES30.glGetError());
                return 0;
            }

            int vertexShader = loadShader(GLES30.GL_VERTEX_SHADER, VERTEX_SHADER);
            if (vertexShader == 0) {
                return 0;
            }

            int fragmentShader = loadShader(GLES30.GL_FRAGMENT_SHADER, FRAGMENT_SHADER);
            if (fragmentShader == 0) {
                return 0;
            }

            GLES30.glAttachShader(program, vertexShader);
            GLES30.glAttachShader(program, fragmentShader);

            // 链接
            GLES30.glLinkProgram(program);

            // 获取链接状态
            IntBuffer linked = IntBuffer.allocate(1);
            GLES30.glGetProgramiv(program, GLES30.GL_LINK_STATUS, linked);
            // 链接失败
            if (linked.get(0) != GLES30.GL_TRUE) {
                IntBuffer infoLen = IntBuffer.allocate(1);
                GLES30.glGetProgramiv(program, GLES30.GL_INFO_LOG_LENGTH, infoLen);
                if (infoLen.get(0) > 0) {
                    String infoLog = GLES30.glGetProgramInfoLog(program);
                    Log.e(TAG, "load program failed: " + infoLog);
                    return 0;
                }

                GLES30.glDeleteProgram(program);
                return 0;
            }

            return program;
        }

        private int loadShader(int type, String shaderStr) {
            int shader = GLES30.glCreateShader(type);
            if (shader <= 0) {
                Log.e(TAG, "load " + (type == GLES30.GL_VERTEX_SHADER ? "vertex" : "fragment")
                        + " shader failed: " + GLES30.glGetError());
                return 0;
            }

            // 加载 shader 源码
            GLES30.glShaderSource(shader, shaderStr);

            // 编译
            GLES30.glCompileShader(shader);

            // 获取编译状态
            IntBuffer compiled = IntBuffer.allocate(1);
            GLES30.glGetShaderiv(shader, GLES30.GL_COMPILE_STATUS, compiled);
            // 编译失败
            if (compiled.get(0) != GLES30.GL_TRUE) {
                IntBuffer infoLen = IntBuffer.allocate(1);
                GLES30.glGetShaderiv(shader, GLES30.GL_INFO_LOG_LENGTH, infoLen);
                if (infoLen.get(0) > 0) {
                    String infoLog = GLES30.glGetShaderInfoLog(shader);
                    Log.e(TAG, "load " + (type == GLES30.GL_VERTEX_SHADER ? "vertex" : "fragment")
                            + " shader failed: " + infoLog);
                    return 0;
                }

                GLES30.glDeleteShader(shader);
                return 0;
            }

            return shader;
        }

    }

}
