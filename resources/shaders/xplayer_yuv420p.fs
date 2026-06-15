#version 330 core
in lowp vec2 xplayer_TexCoord0;

out vec4 xplayer_FragData;

uniform sampler2D xplayer_TextureY;
uniform sampler2D xplayer_TextureU;
uniform sampler2D xplayer_TextureV;

// 0=>BT.601 1=>BT.709
uniform int xplayer_ColorSpace;

void main()
{
    float y = texture(xplayer_TextureY, xplayer_TexCoord0).r;
    float u = texture(xplayer_TextureU, xplayer_TexCoord0).r - 0.5;
    float v = texture(xplayer_TextureV, xplayer_TexCoord0).r - 0.5;

    float r, g, b;
    if (0 == xplayer_ColorSpace)
    {
        r = y + 1.402 * v;
        g = y - 0.344136 * u - 0.714136 * v;
        b = y + 1.772 * u;
    }
	else if (1 == xplayer_ColorSpace)
    {
        r = y + 1.5748 * v;
        g = y - 0.1873 * u - 0.4681 * v;
        b = y + 1.8556 * u;
    }

    xplayer_FragData = vec4(r, g, b, 1.0);
}