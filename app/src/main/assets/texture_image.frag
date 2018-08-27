#version 300 es

precision mediump float;

uniform int iFilterType;
uniform vec3 vFilterColor;
uniform sampler2D sTexture;

in vec2 vTexCoord;

layout(location=0) out vec4 fragColor;

void main() {
    vec4 tmpColor = texture(sTexture,  vTexCoord);
    
    if (iFilterType == 1) { // 灰度图
        float c = tmpColor.r * vFilterColor.r + tmpColor.g * vFilterColor.g + tmpColor.b * vFilterColor.b;
        fragColor = vec4(c, c, c, tmpColor.a);
    } else if (iFilterType == 2) { // 冷暖色
        tmpColor.r = min(1.0, tmpColor.r + vFilterColor.r);
        tmpColor.g = min(1.0, tmpColor.g + vFilterColor.g);
        tmpColor.b = min(1.0, tmpColor.b + vFilterColor.b);
        fragColor = tmpColor;
    } else if (iFilterType == 3) { // 模糊
        tmpColor += texture(sTexture, vec2(vTexCoord.x - vFilterColor.r,vTexCoord.y - vFilterColor.r));
        tmpColor += texture(sTexture, vec2(vTexCoord.x - vFilterColor.r,vTexCoord.y + vFilterColor.r));
        tmpColor += texture(sTexture, vec2(vTexCoord.x + vFilterColor.r,vTexCoord.y - vFilterColor.r));
        tmpColor += texture(sTexture, vec2(vTexCoord.x + vFilterColor.r,vTexCoord.y + vFilterColor.r));
        tmpColor += texture(sTexture, vec2(vTexCoord.x - vFilterColor.g,vTexCoord.y - vFilterColor.g));
        tmpColor += texture(sTexture, vec2(vTexCoord.x - vFilterColor.g,vTexCoord.y + vFilterColor.g));
        tmpColor += texture(sTexture, vec2(vTexCoord.x + vFilterColor.g,vTexCoord.y - vFilterColor.g));
        tmpColor += texture(sTexture, vec2(vTexCoord.x + vFilterColor.g,vTexCoord.y + vFilterColor.g));
        tmpColor += texture(sTexture, vec2(vTexCoord.x - vFilterColor.b,vTexCoord.y - vFilterColor.b));
        tmpColor += texture(sTexture, vec2(vTexCoord.x - vFilterColor.b,vTexCoord.y + vFilterColor.b));
        tmpColor += texture(sTexture, vec2(vTexCoord.x + vFilterColor.b,vTexCoord.y - vFilterColor.b));
        tmpColor += texture(sTexture, vec2(vTexCoord.x + vFilterColor.b,vTexCoord.y + vFilterColor.b));
        tmpColor /= 13.0;
        fragColor = tmpColor;
    } else {
        fragColor = tmpColor;
    }
}