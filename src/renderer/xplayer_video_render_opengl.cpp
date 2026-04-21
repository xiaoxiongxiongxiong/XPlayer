#include "xplayer_video_render_opengl.h"

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

CXPlayerVideoRenderOpengl::CXPlayerVideoRenderOpengl(QWidget * parent)
    : QOpenGLWidget(parent)
{
}

CXPlayerVideoRenderOpengl::~CXPlayerVideoRenderOpengl()
{
    makeCurrent();

    // 释放资源
    if (0 != m_uiVBO)
    {
        glDeleteBuffers(1, &m_uiVBO);
        m_uiVBO = 0;
    }

    if (0 != m_uiVAO)
    {
        glDeleteVertexArrays(1, &m_uiVAO);
        m_uiVAO = 0;
    }

    if (0 != m_uiEBO)
    {
        glDeleteBuffers(1, &m_uiEBO);
        m_uiEBO = 0;
    }

    for (int i = 0; i < 3; i++)
    {
        glDeleteTextures(1, &m_uiTexures[i]);
        m_uiTexures[i] = 0;
    }

    if (0 != m_uiProgram)
    {
        glDeleteProgram(m_uiProgram);
        m_uiProgram = 0;
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
    glUseProgram(0);

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
    // 片段着色器
    GLuint fragment = 0;
    // 顶点着色器
    GLuint vertex = 0;
    
    vertex = compileShader(GL_VERTEX_SHADER, g_vert_str);
    // 创建片段着色器
    fragment = compileShader(GL_FRAGMENT_SHADER, g_frag_str);
    if (0 == vertex || 0 == fragment)
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

    glUseProgram(m_uiProgram);

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
    if (0 != m_uiProgram && !succ)
    {
        glDeleteProgram(m_uiProgram);
        m_uiProgram = 0;
    }

    return succ;
}

bool CXPlayerVideoRenderOpengl::initTextures()
{
    glGenTextures(3, m_uiTexures);
    for (int i = 0; i < 3; ++i)
    {
        glBindTexture(GL_TEXTURE_2D, m_uiTexures[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    glEnable(GL_DEPTH_TEST);

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

bool CXPlayerVideoRenderOpengl::rendererText(const std::string & str)
{
    SDL_Color color = { 255, 0, 0, 255 };

    // 4. 渲染文字到 SDL_Surface
    // TTF_RenderUTF8_Blended 生成带透明通道的 32位 表面
    //SDL_Surface * surface = TTF_RenderUTF8_Blended(m_ptrFontCtx, text, color);
    //if (!surface)
    //{
    //    xpu_format_string(m_strError, "Text render error: %s", TTF_GetError());
    //    TTF_CloseFont(font);
    //    return false;
    //}

    //textWidth = surface->w;
    //textHeight = surface->h;

    //// 5. 生成 OpenGL 纹理
    //glGenTextures(1, &textTexture);
    //glBindTexture(GL_TEXTURE_2D, textTexture);

    //// 设置纹理参数 (线性过滤，防止锯齿)
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    //// 6. 关键：将 SDL_Surface 的数据上传到 OpenGL 显存
    //// SDL_Surface 通常是 SDL_PIXELFORMAT_RGBA32，对应 GL_RGBA
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);

    //// 清理 SDL 资源
    //SDL_FreeSurface(surface);
    return true;
}
