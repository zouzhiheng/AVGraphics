#version 300 es

layout(location=0) in vec4 aPosition;
layout(location=1) in vec4 aCameraTexCoord;
layout(location=2) in vec4 aWatermarkTexCoord;

uniform mat4 mCameraMatrix;
uniform mat4 mWatermarkMatrix;

out vec2 vCameraTexCoord;
out vec2 vWatermarkTexCoord;

void main() {
    vCameraTexCoord = (mCameraMatrix * aCameraTexCoord).xy;
    vWatermarkTexCoord = (mWatermarkMatrix * aWatermarkTexCoord).xy;
    gl_Position = aPosition;
}
