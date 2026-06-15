#version 330 core
layout(location = 0) in vec3 xplayer_Position;
layout(location = 1) in vec2 xplayer_TextureIn;

out vec2 xplayer_TexCoord0;

void main(void)
{
    gl_Position = vec4(xplayer_Position, 1.0);
    xplayer_TexCoord0 = xplayer_TextureIn;
}