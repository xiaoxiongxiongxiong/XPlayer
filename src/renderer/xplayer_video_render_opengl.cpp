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

bool CXPlayerVideoRenderOpengl::resizeImage(int width, int height)
{
    if (width < 1 || height < 1)
    {
        xpu_format_string(_err, "Input image size w * h(%d * %d) is invalid", width, height);
        return false;
    }

    if (m_iWidth.load() != width || m_iHeight.load() != height)
    {
        m_iWidth.store(width);
        m_iHeight.store(height);
        m_blChanged.store(true);
    }

    return true;
}

bool CXPlayerVideoRenderOpengl::renderer(uint8_t * data[8], const std::string & str)
{
    if (nullptr == data[0] || nullptr == data[1] || nullptr == data[2])
    {
        xpu_format_string(_err, "Input param is invalid!");
        return false;
    }

    const int bytes = m_iWidth.load() * m_iHeight.load();
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

    m_iWriteIndex.store((index + 1) % XPLAYER_OPENGL_FRAME_CACHE);
    m_blExist.store(true);
    emit frameReady();

    return true;
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
    glClear(GL_COLOR_BUFFER_BIT);

    if (!m_blExist.load())
        return;

    const auto index = m_iReadIndex.load();
    const auto w = m_iWidth.load();
    const auto h = m_iHeight.load();

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
    m_blExist.store(false);
    m_iReadIndex.store((index + 1) % XPLAYER_OPENGL_FRAME_CACHE);
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
