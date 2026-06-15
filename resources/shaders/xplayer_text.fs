#version 330 core
in vec2 xplayer_TexCoord0;
out vec4 xplayer_FragData;
uniform sampler2D xplayer_TextureStr;
uniform vec4 xplayer_FontColor;
void main()
{
    vec4 texColor = texture(xplayer_TextureStr, xplayer_TexCoord0);
    if(texColor.a < 0.1)
        discard;
    xplayer_FragData = texColor * xplayer_FontColor;
}