#version 330 core

in lowp vec2 xplayer_TexCoord0;
out vec4 xplayer_FragData;

uniform sampler2D xplayer_TextureY;
// NV12 的 UV 是交错的: RG = UV
uniform sampler2D xplayer_TextureUV; 

void main()
{
    float y = texture(xplayer_TextureY, xplayer_TexCoord0).r - 0.0625;
    // 核心修改点：Y 坐标乘以 0.5 以匹配 UV 纹理的实际高度
    vec2 uv = texture(xplayer_TextureUV, xplayer_TexCoord0).rg - vec2(0.5, 0.5);
    
    float u = uv.r;
    float v = uv.g;
	
	float r = y + 1.402 * v;
    float g = y - 0.344 * u - 0.714 * v;
    float b = y + 1.772 * u;
    
    xplayer_FragData = vec4(clamp(r, 0.0, 1.0), clamp(g, 0.0, 1.0), clamp(b, 0.0, 1.0), 1.0);
}