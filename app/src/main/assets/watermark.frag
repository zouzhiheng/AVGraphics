#version 300 es
#extension GL_OES_EGL_image_external_essl3 : require

precision highp float;

uniform samplerExternalOES sCameraTexture;
uniform sampler2D sWatermarkTexture;

in vec2 vCameraTexCoord;
in vec2 vWatermarkTexCoord;

layout(location=0) out vec4 fragColor;

void main() {
    vec4 camera = texture(sCameraTexture, vCameraTexCoord);
    vec4 watermark = texture(sWatermarkTexture, vWatermarkTexCoord);
    // 水印之外的区域显示为相机预览图
    float r = watermark.r + (1.0 - watermark.a) * camera.r;
    float g = watermark.g + (1.0 - watermark.a) * camera.g;
    float b = watermark.b + (1.0 - watermark.a) * camera.b;
    fragColor = vec4(r, g, b, 1.0);
}