#include "xplayer_video_render_opengl.h"

#include <sstream>
#include "SDL2/SDL_ttf.h"

#include "xplayer_utils.h"

static const char * g_vert_str = R"(
    #version 330 core
    layout(location = 0) in vec3 xplayer_Position;
    layout(location = 1) in vec2 xplayer_TextureIn;
    
    out vec2 xplayer_TexCoord0;
    
    void main(void)
    {
        gl_Position = vec4(xplayer_Position, 1.0);
        xplayer_TexCoord0 = xplayer_TextureIn;
    }
)";

static const char * g_frag_str = R"(
    #version 330 core
    in lowp vec2 xplayer_TexCoord0;

    out vec4 xplayer_FragData;

    uniform sampler2D xplayer_TextureY;
    uniform sampler2D xplayer_TextureU;
    uniform sampler2D xplayer_TextureV;
  //  uniform mediump float xplayer_Alpha;
    
    void main()
    {
        float y = texture(xplayer_TextureY, xplayer_TexCoord0).r;
        float u = texture(xplayer_TextureU, xplayer_TexCoord0).r - 0.5;
        float v = texture(xplayer_TextureV, xplayer_TexCoord0).r - 0.5;

        float r = y + 1.402 * v;
        float g = y - 0.344136 * u - 0.714136 * v;
        float b = y + 1.772 * u;

        xplayer_FragData = vec4(r, g, b, 1.0);
    }
)";

static const char * g_text_frag_str = R"(
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
)";

CXPlayerVideoRenderOpengl::CXPlayerVideoRenderOpengl(QWidget * parent)
    : QOpenGLWidget(parent)
{
}

CXPlayerVideoRenderOpengl::~CXPlayerVideoRenderOpengl()
{
    makeCurrent();

    uninitVertices();
    uninitFontVertices();

    for (int i = 0; i < 4; i++)
    {
        glDeleteTextures(1, &m_uiTexures[i]);
        m_uiTexures[i] = 0;
    }

    if (0 != m_uiProgram)
    {
        glDeleteProgram(m_uiProgram);
        m_uiProgram = 0;
    }

    if (0 != m_uiFontProgram)
    {
        glDeleteProgram(m_uiFontProgram);
        m_uiFontProgram = 0;
    }

    doneCurrent();
}

bool CXPlayerVideoRenderOpengl::create(const void * wnd, int wnd_width, int wnd_height, int frm_width, int frm_height)
{
    if (nullptr == wnd || wnd_width <= 0 || wnd_height <= 0 || frm_width <= 0 || frm_height <= 0)
    {
        xpu_format_string(m_strError, "Input param is invalid");
        return false;
    }

    m_iFrameWidth.store(frm_width);
    m_iFrameHeight.store(frm_height);
    m_blChanged.store(true);

    return true;
}

void CXPlayerVideoRenderOpengl::destroy()
{
    uninitFontContext();
}

bool CXPlayerVideoRenderOpengl::initFontContext(const std::string & path, int size)
{
    return openFont(path, size);
}

void CXPlayerVideoRenderOpengl::uninitFontContext()
{
    closeFont();
}

bool CXPlayerVideoRenderOpengl::resizeWindow(int width, int height)
{
    return adjust(width, height, true);;
}

bool CXPlayerVideoRenderOpengl::resizeImage(int width, int height)
{
    return adjust(width, height, false);
}

bool CXPlayerVideoRenderOpengl::renderer(uint8_t * data[8], int linesize[8], const std::string & str)
{
    if (nullptr == data[0] || nullptr == data[1] || nullptr == data[2])
    {
        xpu_format_string(_err, "Input param is invalid!");
        return false;
    }

    const int bytes = m_iFrameWidth.load() * m_iFrameHeight.load();
    const auto index = m_iWriteIndex.load();
    if (m_ucCache[index][0].size() != bytes)
    {
        m_ucCache[index][0].resize(bytes);
        m_ucCache[index][1].resize(bytes / 4);
        m_ucCache[index][2].resize(bytes / 4);
    }

    memcpy(m_ucCache[index][0].data(), data[0], bytes);
    memcpy(m_ucCache[index][1].data(), data[1], bytes / 4);
    memcpy(m_ucCache[index][2].data(), data[2], bytes / 4);
    m_ucCache[index][3] = QByteArray(str.c_str(), str.size());

    m_iWriteIndex.store((index + 1) % XPLAYER_OPENGL_FRAME_CACHE);

    //emit frameReady();
    update();

    return true;
}

void CXPlayerVideoRenderOpengl::clear()
{
    m_iWriteIndex.store(0);
    m_iReadIndex.store(0);
    update();
   // emit frameReady();
}

XPLAYER_VIDEO_RENDERER_TYPE CXPlayerVideoRenderOpengl::getType() const
{
    return XPLAYER_VIDEO_RENDERER_OPENGL;
}

const char * CXPlayerVideoRenderOpengl::err() const
{
    return _err.c_str();
}

void CXPlayerVideoRenderOpengl::initializeGL()
{
    // 【重要】初始化 OpenGL 函数解析器
    initializeOpenGLFunctions();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    initShader();

    initTextures();

    initVertices();

    initFontVertices();
}

void CXPlayerVideoRenderOpengl::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (m_iReadIndex.load() == m_iWriteIndex.load())
        return;

    const auto index = m_iReadIndex.load();
    const auto w = m_iFrameWidth.load();
    const auto h = m_iFrameHeight.load();

    // 1. 使用着色器程序
    glUseProgram(m_uiProgram);

    // 2. 绑定 VAO
    glBindVertexArray(m_uiVAO);

    // 更新 Y 纹理
    glActiveTexture(GL_TEXTURE0); // 激活纹理单元 0
    glBindTexture(GL_TEXTURE_2D, m_uiTexures[0]);
    if (m_blChanged.load())
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, m_ucCache[index][0].constData());
    else
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RED, GL_UNSIGNED_BYTE, m_ucCache[index][0].constData());
    // 将纹理单元 0 绑定到着色器中的 uniform sampler2D textureY
    glUniform1i(glGetUniformLocation(m_uiProgram, "xplayer_TextureY"), 0);

    // 更新 U 纹理
    glActiveTexture(GL_TEXTURE1); // 激活纹理单元 1
    glBindTexture(GL_TEXTURE_2D, m_uiTexures[1]);
    if (m_blChanged.load())
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w / 2, h / 2, 0, GL_RED, GL_UNSIGNED_BYTE, m_ucCache[index][1].constData());
    else
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w / 2, h / 2, GL_RED, GL_UNSIGNED_BYTE, m_ucCache[index][1].constData());
    glUniform1i(glGetUniformLocation(m_uiProgram, "xplayer_TextureU"), 1);

    // 更新 V 纹理
    glActiveTexture(GL_TEXTURE2); // 激活纹理单元 2
    glBindTexture(GL_TEXTURE_2D, m_uiTexures[2]);
    if (m_blChanged.load())
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w / 2, h / 2, 0, GL_RED, GL_UNSIGNED_BYTE, m_ucCache[index][2].constData());
    else
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w / 2, h / 2, GL_RED, GL_UNSIGNED_BYTE, m_ucCache[index][2].constData());
    glUniform1i(glGetUniformLocation(m_uiProgram, "xplayer_TextureV"), 2);

    // 3. 绘制
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // 4. 解绑（可选，保持状态整洁）
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glUseProgram(0);

    rendererText(m_ucCache[index][3].toStdString());
    m_blChanged.store(false);
    m_iReadIndex.store((index + 1) % XPLAYER_OPENGL_FRAME_CACHE);
}

void CXPlayerVideoRenderOpengl::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

GLuint CXPlayerVideoRenderOpengl::compileShader(GLenum type, const char * src)
{
    GLint status = 0;

    auto shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    // 检查编译错误
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (GL_FALSE == status)
    {
        char buff[512] = {};
        glGetShaderInfoLog(shader, 512, nullptr, buff);
        xpu_format_string(_err, "%s", buff);
        glDeleteShader(shader);
        shader = 0;
    }

    return shader;
}

bool CXPlayerVideoRenderOpengl::initShader()
{
    bool succ = false;
    GLint status = 0;
    // 顶点着色器
    GLuint vertex = 0;
    // 片段着色器
    GLuint fragment = 0;
    // 字体片段
    GLuint font_fragment = 0;
    
    vertex = compileShader(GL_VERTEX_SHADER, g_vert_str);
    // 创建片段着色器
    fragment = compileShader(GL_FRAGMENT_SHADER, g_frag_str);
    font_fragment = compileShader(GL_FRAGMENT_SHADER, g_text_frag_str);
    if (0 == vertex || 0 == fragment || 0 == font_fragment)
        goto end;

    // 链接着色器程序
    m_uiProgram = glCreateProgram();
    glAttachShader(m_uiProgram, vertex);
    glAttachShader(m_uiProgram, fragment);
    glBindAttribLocation(m_uiProgram, m_uiVertexLocation, "xplayer_Position");
    glBindAttribLocation(m_uiProgram, m_uiTextureLocation, "xplayer_TextureIn");
    glLinkProgram(m_uiProgram);

    // 检查链接错误
    glGetProgramiv(m_uiProgram, GL_LINK_STATUS, &status);
    if (GL_FALSE == status)
    {
        char buff[512] = {};
        glGetProgramInfoLog(m_uiProgram, 512, nullptr, buff);
        xpu_format_string(_err, "%s", buff);
        goto end;
    }

    // 字体
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_uiFontProgram = glCreateProgram();
    glAttachShader(m_uiFontProgram, vertex);
    glAttachShader(m_uiFontProgram, font_fragment);
    glLinkProgram(m_uiFontProgram);

    // 检查链接错误
    glGetProgramiv(m_uiFontProgram, GL_LINK_STATUS, &status);
    if (GL_FALSE == status)
    {
        char buff[512] = {};
        glGetProgramInfoLog(m_uiFontProgram, 512, nullptr, buff);
        xpu_format_string(_err, "%s", buff);
        goto end;
    }

    succ = true;

end:
    if (0 != vertex)
    {
        glDeleteShader(vertex);
        vertex = 0;
    }
    if (0 != fragment)
    {
        glDeleteShader(fragment);
        fragment = 0;
    }
    if (0 != font_fragment)
    {
        glDeleteShader(font_fragment);
        font_fragment = 0;
    }
    if (0 != m_uiProgram && !succ)
    {
        glDeleteProgram(m_uiProgram);
        m_uiProgram = 0;
    }
    if (0 != m_uiFontProgram && !succ)
    {
        glDeleteProgram(m_uiFontProgram);
        m_uiFontProgram = 0;
    }

    return succ;
}

bool CXPlayerVideoRenderOpengl::initTextures()
{
    glGenTextures(4, m_uiTexures);
    for (int i = 0; i < 4; ++i)
    {
        glBindTexture(GL_TEXTURE_2D, m_uiTexures[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    return true;
}

void CXPlayerVideoRenderOpengl::initVertices()
{
    // 设置顶点数据
    const float vertices[] =
    {
        // 位置              // 纹理坐标
        -1.0f, -1.0f, 0.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 0.0f, 1.0f, 0.0f
    };

    const uint indices[] =
    {
        0, 1, 2,
        1, 2, 3
    };

    glGenVertexArrays(1, &m_uiVAO);
    glGenBuffers(1, &m_uiVBO);
    glGenBuffers(1, &m_uiEBO);

    glBindVertexArray(m_uiVAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_uiVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_uiEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // 位置属性 (location = 0)
    glVertexAttribPointer(m_uiVertexLocation, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(m_uiVertexLocation);

    // 纹理坐标属性 (location = 1)
    glVertexAttribPointer(m_uiTextureLocation, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(m_uiTextureLocation);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void CXPlayerVideoRenderOpengl::uninitVertices()
{
    if (0 != m_uiVAO)
    {
        glDeleteVertexArrays(1, &m_uiVAO);
        m_uiVAO = 0;
    }

    if (0 != m_uiVBO)
    {
        glDeleteBuffers(1, &m_uiVBO);
        m_uiVBO = 0;
    }

    if (0 != m_uiEBO)
    {
        glDeleteBuffers(1, &m_uiEBO);
        m_uiEBO = 0;
    }
}

void CXPlayerVideoRenderOpengl::initFontVertices()
{
    const uint indices[] =
    {
        0, 1, 3,
        1, 2, 3
    };

    // 字体
    glGenVertexArrays(1, &m_uiFontVAO);
    glGenBuffers(1, &m_uiFontVBO);
    glGenBuffers(1, &m_uiFontEBO);

    glBindVertexArray(m_uiFontVAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_uiFontVBO);
    glBufferData(GL_ARRAY_BUFFER, 4 * 5 * sizeof(float), nullptr, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_uiFontEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(m_uiVertexLocation, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(m_uiVertexLocation);

    glVertexAttribPointer(m_uiTextureLocation, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(m_uiTextureLocation);

    glBindVertexArray(0); // 解绑字体 VAO
}

void CXPlayerVideoRenderOpengl::uninitFontVertices()
{
    if (0 != m_uiFontVAO)
    {
        glDeleteVertexArrays(1, &m_uiFontVAO);
        m_uiFontVAO = 0;
    }

    if (0 != m_uiFontVBO)
    {
        glDeleteBuffers(1, &m_uiFontVBO);
        m_uiFontVBO = 0;
    }

    if (0 != m_uiFontEBO)
    {
        glDeleteBuffers(1, &m_uiFontEBO);
        m_uiFontEBO = 0;
    }
}

bool CXPlayerVideoRenderOpengl::rendererText(const std::string & str)
{
    if (str.empty() || nullptr == m_ptrFontCtx)
        return true;

    glUseProgram(m_uiFontProgram);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, m_uiTexures[3]);
    glUniform1i(glGetUniformLocation(m_uiFontProgram, "xplayer_TextureStr"), 3);
    glUniform4f(glGetUniformLocation(m_uiFontProgram, "xplayer_FontColor"), 1.0f, 0.0f, 0.0f, 1.0f);

    glBindVertexArray(m_uiFontVAO);

    // 行距
    const float line_space = 12.0f;
    // 边距
    const float margin = 10.0f;
    const auto screen_ratio_x = 2.0f / static_cast<float>(this->width());
    const auto screen_ratio_y = 2.0f / static_cast<float>(this->height());
    float cur_height = margin;

    // 字体高度
    int font_height = TTF_FontHeight(m_ptrFontCtx);

    std::string val;
    std::istringstream iss(str);
    while (getline(iss, val, '\n'))
    {
        if (val.empty())
            continue;

        SDL_Color font_color = { 255, 0, 0, 255 };
        SDL_Surface * surface = TTF_RenderUTF8_Blended(m_ptrFontCtx, val.c_str(), font_color);
        if (nullptr == surface)
        {
            xpu_format_string(m_strError, "TTF_RenderUTF8_Blended error: %s", TTF_GetError());
            return false;
        }

        const auto width = surface->w;
        const auto height = surface->h;
        std::vector<uint8_t> pixel_data;
        pixel_data.reserve(width * height * 4);

        for (int y = 0; y < height; y++)
        {
            uint8_t * row = reinterpret_cast<uint8_t *>(surface->pixels) + y * surface->pitch;
            for (int x = 0; x < width; x++)
            {
                Uint32 pixel = ((Uint32 *)row)[x];
                Uint8 r, g, b, a;
                SDL_GetRGBA(pixel, surface->format, &r, &g, &b, &a);

                pixel_data.push_back(r);
                pixel_data.push_back(g);
                pixel_data.push_back(b);
                pixel_data.push_back(a);
            }
        }

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel_data.data());

        SDL_FreeSurface(surface);

        // 左上坐标
        float x_start = -1.0f + margin * screen_ratio_x;
        float x_end = -1.0f + (margin + width) * screen_ratio_x;
        float y_top = 1.0f - cur_height * screen_ratio_y;
        float y_bottom = 1.0f - (cur_height + height) * screen_ratio_y;

        float vertices[] = {
            x_start, y_top,    0.0f, 0.0f, 0.0f, // 0: 左上
            x_end,   y_top,    0.0f, 1.0f, 0.0f, // 1: 右上
            x_end,   y_bottom, 0.0f, 1.0f, 1.0f, // 2: 右下
            x_start, y_bottom, 0.0f, 0.0f, 1.0f  // 3: 左下
        };

        // 5. 更新 VBO
        glBindBuffer(GL_ARRAY_BUFFER, m_uiFontVBO);
        // 使用 glBufferSubData 比 glBufferData 更安全，不需要重新分配内存
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        cur_height += (line_space + height);
    }

    glBindVertexArray(0);
    glUseProgram(0);

    return true;
}
