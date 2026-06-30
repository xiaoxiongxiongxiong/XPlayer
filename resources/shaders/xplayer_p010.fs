#version 330 core

in lowp vec2 xplayer_TexCoord0;
out vec4 xplayer_FragData;

uniform sampler2D xplayer_TextureY;
// NV12 的 UV 是交错的: RG = UV
uniform sampler2D xplayer_TextureUV; 

void main()
{
    vec4 yCol = texture(xplayer_TextureY, xplayer_TexCoord0);
    vec4 uvCol = texture(xplayer_TextureUV, xplayer_TexCoord0);

    // 2. 将 2 个 8-bit 数据组合成 16-bit 并归一化 (默认小端序)
    float val = 255.0 * yCol.r + yCol.a * 255.0 * pow(2.0, 8.0);
    float yVal = val / 65535.0 - 0.063;

    // 3. 提取 V 分量 (RG 通道)
    val = 255.0 * uvCol.r + uvCol.g * 255.0 * pow(2.0, 8.0);
    float vVal = val / 65535.0 - 0.502;

    // 4. 提取 U 分量 (BA 通道)
    val = 255.0 * uvCol.b + uvCol.a * 255.0 * pow(2.0, 8.0);
    float uVal = val / 65535.0 - 0.502;

    // 5. YUV 转 RGB 矩阵运算
    highp vec3 rgb = mat3(1.164, 1.164, 1.164,
                          0.0, -0.392, 2.017,
                          1.596, -0.813, 0.0) * vec3(yVal, uVal, vVal);

    xplayer_FragData = vec4(rgb, 1.0);
}