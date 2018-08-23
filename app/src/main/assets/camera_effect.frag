#version 300 es
#extension GL_OES_EGL_image_external_essl3 : require

precision highp float;

uniform samplerExternalOES sCameraTexture;
uniform sampler2D sWatermarkTexture;
uniform sampler2D sTextTexture;

in vec2 vCameraTexCoord;
in vec2 vWatermarkTexCoord;
in vec2 vTextTexCoord;

layout(location=0) out vec4 cameraColor;
// layout(location=1) out vec4 watermarkColor;
// layout(location=2) out vec4 textColor;

void main() {
    cameraColor = texture(sCameraTexture, vCameraTexCoord);
    // watermarkColor = texture(sWatermarkTexture, vWatermarkTexCoord);
    // textColor = texture(sTextTexture, vTextTexCoord);
}