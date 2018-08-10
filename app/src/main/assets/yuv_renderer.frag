#version 300 es

precision highp float;

uniform sampler2D yTexture;
uniform sampler2D uTexture;
uniform sampler2D vTexture;

in vec2 vTexCoord;

layout(location=0) out vec4 fragColor;

void main() {
    highp float y = texture(yTexture, vTexCoord).r;
    highp float u = texture(uTexture, vTexCoord).r - 0.5;
    highp float v = texture(vTexture, vTexCoord).r - 0.5;

    highp float r = y + 1.402 * v;
    highp float g = y - 0.344 * u - 0.714 * v;
    highp float b = y + 1.772 * u;
    fragColor = vec4(r, g, b, 1.0);
}