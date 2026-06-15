#version 330 core

in lowp vec2 xplayer_TexCoord0;
out vec4 xplayer_FragData;

uniform sampler2D xplayer_TextureY;
// NV12 的 UV 是交错的: RG = UV
uniform sampler2D xplayer_TextureUV; 

void main()
{
    float y = texture(xplayer_TextureY, xplayer_TexCoord0).r;
    vec2 uv = texture(xplayer_TextureUV, xplayer_TexCoord0).rg;
    float u = uv.r - 0.5;
    float v = uv.g - 0.5;
    
    xplayer_FragData = vec4(
        y + 1.5748 * v,
        y - 0.1873 * u - 0.4681 * v,
        y + 1.8556 * u,
        1.0
    );
}