#include "xplayer_video_render_opengl.h"

#include <QThread>
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

    _is_over.store(true);

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

void CXPlayerVideoRenderOpengl::setSize(int width, int height)
{
    if (width < 1 || height < 1)
        return;

    if (m_iWidth.load() != width || m_iHeight.load() != height)
    {
        m_iWidth.store(width);
        m_iHeight.store(height);
        _changed.store(true);
    }
}

void CXPlayerVideoRenderOpengl::renderer(const uint8_t * y, const uint8_t * u, const uint8_t * v, const std::string & str)
{
    if (nullptr == y || nullptr == u || nullptr == v)
        return;

    glBindTexture(GL_TEXTURE_2D, m_uiTexures[0]);
    if (_changed.load())
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_iWidth.load(), m_iHeight.load(), 0, GL_RED, GL_UNSIGNED_BYTE, y);
    else
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_iWidth.load(), m_iHeight.load(), GL_RED, GL_UNSIGNED_BYTE, y);

    glBindTexture(GL_TEXTURE_2D, m_uiTexures[1]);
    if (_changed.load())
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_iWidth.load() / 2, m_iHeight.load() / 2, 0, GL_RED, GL_UNSIGNED_BYTE, u);
    else
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_iWidth.load() / 2, m_iHeight.load() / 2, GL_RED, GL_UNSIGNED_BYTE, u);

    glBindTexture(GL_TEXTURE_2D, m_uiTexures[2]);
    if (_changed.load())
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_iWidth.load() / 2, m_iHeight.load() / 2, 0, GL_RED, GL_UNSIGNED_BYTE, v);
    else
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_iWidth.load() / 2, m_iHeight.load() / 2, GL_RED, GL_UNSIGNED_BYTE, v);

    _changed.store(false);
    _is_over.store(false);

    repaint();

    while (!_is_over.load())
    {
        QThread::usleep(1);
    }
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
    glClear(GL_COLOR_BUFFER_BIT);

    if (_is_over.load())
        return;

    // 1. 使用着色器程序
    glUseProgram(m_uiProgram);

    // 2. 绑定 VAO
    glBindVertexArray(m_uiVAO);

    // 更新 Y 纹理
    glActiveTexture(GL_TEXTURE0); // 激活纹理单元 0
    glBindTexture(GL_TEXTURE_2D, m_uiTexures[0]);
    // 将纹理单元 0 绑定到着色器中的 uniform sampler2D textureY
    glUniform1i(glGetUniformLocation(m_uiProgram, "xplayer_TextureY"), 0);

    // 更新 U 纹理
    glActiveTexture(GL_TEXTURE1); // 激活纹理单元 1
    glBindTexture(GL_TEXTURE_2D, m_uiTexures[1]);
    glUniform1i(glGetUniformLocation(m_uiProgram, "xplayer_TextureU"), 1);

    // 更新 V 纹理
    glActiveTexture(GL_TEXTURE2); // 激活纹理单元 2
    glBindTexture(GL_TEXTURE_2D, m_uiTexures[2]);
    glUniform1i(glGetUniformLocation(m_uiProgram, "xplayer_TextureV"), 2);

    // 3. 绘制
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // 4. 解绑（可选，保持状态整洁）
    glBindVertexArray(0);
    glUseProgram(0);

    _is_over.store(true);
}

void CXPlayerVideoRenderOpengl::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

GLuint CXPlayerVideoRenderOpengl::complileShader(GLenum type, const char * src)
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
    
    vertex = complileShader(GL_VERTEX_SHADER, g_vert_str);
    // 创建片段着色器
    fragment = complileShader(GL_FRAGMENT_SHADER, g_frag_str);
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
