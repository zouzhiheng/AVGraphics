#version 300 es

layout(location=0) in vec4 aPosition;
layout(location=1) in vec4 aCameraTexCoord;
layout(location=2) in vec4 aWatermarkTexCoord;
layout(location=3) in vec4 aTextTexCoord;

uniform mat4 mMatrix;

out vec2 vCameraTexCoord;
out vec2 vWatermarkTexCoord;
out vec2 vTextTexCoord;

void main() {
    vCameraTexCoord = (mMatrix * aCameraTexCoord).xy;
    vWatermarkTexCoord = aWatermarkTexCoord.xy;
    vTextTexCoord = aTextTexCoord.xy;
    gl_Position = aPosition;
}
