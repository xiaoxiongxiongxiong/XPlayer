#include "xplayer_video_render_opengl.h"

#include "xplayer_utils.h"

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
        vec3 yuv, rgb;
        yuv.x = texture(xplayer_TextureY, xplayer_TexCoord0).r;
        yuv.y = texture(xplayer_TextureU, xplayer_TexCoord0).r - 0.5;
        yuv.z = texture(xplayer_TextureV, xplayer_TexCoord0).r - 0.5;
        rgb = mat3(1.0,       1.0,      1.0, 
                   0.0,      -0.337633, 1.732446, 
                   1.370705, -0.698001, 0.0) * yuv;
        xplayer_FragData = vec4(rgb, 1.0);
    }
)";

static const char * g_vert_str = R"(
    #version 330 core
    layout(location = 0) in vec2 xplayer_Vertex;
    layout(location = 1) in vec2 xplayer_TextureIn;
    
    out vec2 xplayer_TexCoord0;
    
    void main(void)
    {
        gl_Position = vec4(xplayer_Vertex, 0.0, 1.0);
        xplayer_TexCoord0 = xplayer_TextureIn;
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
    m_iWidth.store(width);
    m_iHeight.store(height);
}

void CXPlayerVideoRenderOpengl::renderer(const uint8_t * y, const uint8_t * u, const uint8_t * v, const std::string & str)
{
    if (nullptr == y || nullptr == u || nullptr == v || 0 == m_uiTexures[0])
        return;

    const auto bytes = m_iWidth.load() * m_iHeight.load();
    m_yData = QByteArray(reinterpret_cast<const char *>(y), bytes);
    m_uData = QByteArray(reinterpret_cast<const char *>(u), bytes / 4);
    m_vData = QByteArray(reinterpret_cast<const char *>(v), bytes / 4);

    auto aaa = m_yData.isEmpty();

    repaint();
    //update(); // 触发 paintGL 重绘
}

void CXPlayerVideoRenderOpengl::initializeGL()
{
    // 【重要】初始化 OpenGL 函数解析器
    initializeOpenGLFunctions();

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    initShader();

    initTextures();

    initVertices();
}

void CXPlayerVideoRenderOpengl::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);

    if (m_yData.isEmpty())
        return;

    glClear(GL_COLOR_BUFFER_BIT);

    // 1. 使用着色器程序
    glUseProgram(m_uiProgram);

    // 2. 绑定 VAO
    glBindVertexArray(m_uiVAO);

    // --- 关键步骤：更新纹理数据 ---
    const auto w = m_iWidth.load();
    const auto h = m_iHeight.load();

    // 更新 Y 纹理
    glActiveTexture(GL_TEXTURE0); // 激活纹理单元 0
    glBindTexture(GL_TEXTURE_2D, m_uiTexures[0]);
    // 使用 glTexSubImage2D 更新数据，比 glTexImage2D 效率更高
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RED, GL_UNSIGNED_BYTE, m_yData.constData());
    // 将纹理单元 0 绑定到着色器中的 uniform sampler2D textureY
    glUniform1i(glGetUniformLocation(m_uiProgram, "xplayer_TextureY"), 0);

    // 更新 U 纹理
    glActiveTexture(GL_TEXTURE1); // 激活纹理单元 1
    glBindTexture(GL_TEXTURE_2D, m_uiTexures[1]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w / 2, h / 2, GL_RED, GL_UNSIGNED_BYTE, m_uData.constData());
    glUniform1i(glGetUniformLocation(m_uiProgram, "xplayer_TextureU"), 1);

    // 更新 V 纹理
    glActiveTexture(GL_TEXTURE2); // 激活纹理单元 2
    glBindTexture(GL_TEXTURE_2D, m_uiTexures[2]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w / 2, h / 2, GL_RED, GL_UNSIGNED_BYTE, m_vData.constData());
    glUniform1i(glGetUniformLocation(m_uiProgram, "xplayer_TextureV"), 2);

    // 3. 绘制
    //glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
    ////glViewport(0, 0, w, h);
    //glClearColor(0.f, 1.f, 0.f, 1.f);
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    //// 4. 解绑（可选，保持状态整洁）
    glBindVertexArray(0);
    glUseProgram(0);
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
    glBindAttribLocation(m_uiProgram, m_uiVertexLocation, "xplayer_Vertex");
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

    glBindTexture(GL_TEXTURE_2D, 0);
    //glEnable(GL_DEPTH_TEST);

    return true;
}

void CXPlayerVideoRenderOpengl::initVertices()
{
    float vertices[] =
    {
        -1.0f, -1.0f,   0.0f, 1.0f, // 左下
         1.0f, -1.0f,   1.0f, 1.0f, // 右下
         1.0f,  1.0f,   1.0f, 0.0f, // 右上
        -1.0f,  1.0f,   0.0f, 0.0f  // 左上
    };

    glGenVertexArrays(1, &m_uiVAO);
    glGenBuffers(1, &m_uiVBO);

    glBindVertexArray(m_uiVAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_uiVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // 位置属性 (location = 0)
    glVertexAttribPointer(m_uiVertexLocation, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(m_uiVertexLocation);

    // 纹理坐标属性 (location = 1)
    glVertexAttribPointer(m_uiTextureLocation, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(m_uiTextureLocation);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}
