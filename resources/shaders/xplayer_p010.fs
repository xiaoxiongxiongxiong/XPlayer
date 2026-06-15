#version 330 core

in lowp vec2 xplayer_TexCoord0;
out vec4 xplayer_FragData;

uniform sampler2D xplayer_TextureY; // R16
uniform sampler2D xplayer_TextureU; // R16
uniform sampler2D xplayer_TextureV; // R16

void main()
{
    float y = texture(xplayer_TextureY, xplayer_TexCoord0).r;
    float u = texture(xplayer_TextureU, xplayer_TexCoord0).r - 0.5;
    float v = texture(xplayer_TextureV, xplayer_TexCoord0).r - 0.5;
    
    xplayer_FragData = vec4(
        y + 1.5748 * v,
        y - 0.1873 * u - 0.4681 * v,
        y + 1.8556 * u,
        1.0
    );
}