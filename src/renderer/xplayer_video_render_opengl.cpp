#include "xplayer_video_render_opengl.h"

#include <sstream>
#include <functional>
#include "SDL2/SDL_ttf.h"

#include "xplayer_utils.h"

CXPlayerVideoRenderOpengl::CXPlayerVideoRenderOpengl(QWidget * parent)
    : QOpenGLWidget(parent)
{
    _format.store(XPLAYER_PIXEL_FORMAT_P010);
    initOptions();
}

CXPlayerVideoRenderOpengl::~CXPlayerVideoRenderOpengl()
{
    makeCurrent();

    uninitShader();
    uninitTextures();
    uninitVertices();
    uninitFontVertices();

    doneCurrent();
}

bool CXPlayerVideoRenderOpengl::supportedPixelFormat(std::vector<XPLAYER_PIXEL_FORMAT_TYPE> & formats)
{
    formats.clear();

    formats.push_back(XPLAYER_PIXEL_FORMAT_YUV420P);
    formats.push_back(XPLAYER_PIXEL_FORMAT_YUY2);
    formats.push_back(XPLAYER_PIXEL_FORMAT_UYVY);
    formats.push_back(XPLAYER_PIXEL_FORMAT_YVYU);
    formats.push_back(XPLAYER_PIXEL_FORMAT_YUV420P10);
    formats.push_back(XPLAYER_PIXEL_FORMAT_NV12);
    formats.push_back(XPLAYER_PIXEL_FORMAT_NV21);
    formats.push_back(XPLAYER_PIXEL_FORMAT_P010);

    return true;
}

void CXPlayerVideoRenderOpengl::setFontPath(const std::string & path)
{
    _font_path = path;
}

void CXPlayerVideoRenderOpengl::setFontSize(int size)
{
    _font_size = size;
}

void CXPlayerVideoRenderOpengl::setFontColor(const xplayer_color_t & color)
{
    _font_color = color;

    _color.setX(static_cast<float>(color.red) / 255.f);
    _color.setY(static_cast<float>(color.green) / 255.f);
    _color.setZ(static_cast<float>(color.blue) / 255.f);
    _color.setW(static_cast<float>(color.alpha) / 255.f);
}

bool CXPlayerVideoRenderOpengl::create(const void * wnd, int width, int height)
{
    if (nullptr == wnd || width <= 0 || height <= 0)
    {
        xpu_format_string(_err, "Input param is invalid");
        return false;
    }

    _write_index.store(0);
    _read_index.store(0);

    _write_times.store(0);
    _read_times.store(0);

    return openFont(_font_path, _font_size);
}

void CXPlayerVideoRenderOpengl::destroy()
{
    closeFont();
}

bool CXPlayerVideoRenderOpengl::resize(int width, int height)
{
    return adjust(width, height, true);;
}

bool CXPlayerVideoRenderOpengl::renderer(int width, int height, XPLAYER_PIXEL_FORMAT_TYPE format, 
                                         uint8_t * data[8], int linesize[8], const std::string & str)
{
    auto found = _opts.find(format);
    if (_opts.end() == found)
    {
        xpu_format_string(_err, "Unsupported format %d", format);
        return false;
    }

    const auto index = _write_index.load();
    _cache[index]._width = width;
    _cache[index]._height = height;
    _cache[index]._format = format;
    if (!(*found).second.cache(index, data, linesize))
        return false;
    _cache[index]._data[3] = QByteArray(str.c_str(), str.size());

    _write_index.store((index + 1) % XPLAYER_OPENGL_FRAME_CACHE);

    _write_times++;

    update();

    return true;
}

void CXPlayerVideoRenderOpengl::clear()
{
    _write_index.store(0);
    _read_index.store(0);
    _write_times.store(0);
    _read_times.store(0);
    update();
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

    initShader(_format.load());

    initTextures(_format.load());

    initVertices();

    initFontVertices();
}

void CXPlayerVideoRenderOpengl::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (_read_times.load() == _write_times.load())
        return;

    const auto index = _read_index.load();
    const auto w = _cache[index]._width;
    const auto h = _cache[index]._height;
    const auto format = _cache[index]._format;

    if (_format.load() != format)
    {
        uninitShader();
        initShader(format);
        uninitTextures(true);
        initTextures(format, true);
        _format.store(format);
    }

    if (_img_width.load() != w || _img_height.load() != h)
    {
        _img_width.store(w);
        _img_height.store(h);
        _changed.store(true);
    }

    // 1. 使用着色器程序
    glUseProgram(_program);

    auto found = _opts.find(_format.load());
    if (_opts.end() != found)
        (*found).second.render(w, h, index);

    // 4. 解绑（可选，保持状态整洁）
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glUseProgram(0);

    rendererText(_cache[index]._data[3].toStdString());
    _changed.store(false);
    _read_index.store((index + 1) % XPLAYER_OPENGL_FRAME_CACHE);
    _read_times++;
}

void CXPlayerVideoRenderOpengl::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

void CXPlayerVideoRenderOpengl::initOptions()
{
    xplayer_opengl_option_t yuv420p{};
    yuv420p.cnt = 3;
    yuv420p.cache = std::bind(&CXPlayerVideoRenderOpengl::cacheYUV420P, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    yuv420p.render = std::bind(&CXPlayerVideoRenderOpengl::renderYUV420P, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _opts.emplace(XPLAYER_PIXEL_FORMAT_YUV420P, yuv420p);

    xplayer_opengl_option_t yuy2{};
    yuy2.cnt = 3;
    yuy2.cache = std::bind(&CXPlayerVideoRenderOpengl::cacheYUY2, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    yuy2.render = std::bind(&CXPlayerVideoRenderOpengl::renderYUY2, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _opts.emplace(XPLAYER_PIXEL_FORMAT_YUY2, yuy2);

    xplayer_opengl_option_t uyvy{};
    uyvy.cnt = 3;
    uyvy.cache = std::bind(&CXPlayerVideoRenderOpengl::cacheUYVY, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    uyvy.render = std::bind(&CXPlayerVideoRenderOpengl::renderUYVY, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _opts.emplace(XPLAYER_PIXEL_FORMAT_UYVY, uyvy);

    xplayer_opengl_option_t yvyu{};
    yvyu.cnt = 3;
    yvyu.cache = std::bind(&CXPlayerVideoRenderOpengl::cacheYVYU, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    yvyu.render = std::bind(&CXPlayerVideoRenderOpengl::renderYVYU, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _opts.emplace(XPLAYER_PIXEL_FORMAT_YVYU, yvyu);

    xplayer_opengl_option_t yuv420p10{};
    yuv420p10.cnt = 3;
    yuv420p10.cache = std::bind(&CXPlayerVideoRenderOpengl::cacheYUV420P10, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    yuv420p10.render = std::bind(&CXPlayerVideoRenderOpengl::renderYUV420P10, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _opts.emplace(XPLAYER_PIXEL_FORMAT_YUV420P10, yuv420p10);

    xplayer_opengl_option_t nv12{};
    nv12.cnt = 2;
    nv12.cache = std::bind(&CXPlayerVideoRenderOpengl::cacheNV12, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    nv12.render = std::bind(&CXPlayerVideoRenderOpengl::renderNV12, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _opts.emplace(XPLAYER_PIXEL_FORMAT_NV12, nv12);

    xplayer_opengl_option_t nv21{};
    nv21.cnt = 2;
    nv21.cache = std::bind(&CXPlayerVideoRenderOpengl::cacheNV21, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    nv21.render = std::bind(&CXPlayerVideoRenderOpengl::renderNV21, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _opts.emplace(XPLAYER_PIXEL_FORMAT_NV21, nv21);

    xplayer_opengl_option_t p010{};
    p010.cnt = 3;
    p010.cache = std::bind(&CXPlayerVideoRenderOpengl::cacheP010, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    p010.render = std::bind(&CXPlayerVideoRenderOpengl::renderP010, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _opts.emplace(XPLAYER_PIXEL_FORMAT_P010, p010);
}

GLuint CXPlayerVideoRenderOpengl::compileShader(GLenum type, const std::string & path)
{
    GLint status = 0;

    std::vector<char> buff;
    if (!xpu_file2str(path, buff, _err))
        return 0;
    buff.push_back('\0');

    const auto * src = buff.data();
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

bool CXPlayerVideoRenderOpengl::initShader(const XPLAYER_PIXEL_FORMAT_TYPE & format)
{
    bool succ = false;
    // 顶点着色器
    GLuint vertex = compileShader(GL_VERTEX_SHADER, "shaders/xplayer_common.vs");
    if (0 == vertex)
        return false;

    switch (format)
    {
    case XPLAYER_PIXEL_FORMAT_YUV420P:
    {
        succ = initShader("shaders/xplayer_yuv420p.fs", vertex, _program);
        _locs[0] = glGetUniformLocation(_program, "xplayer_TextureY");
        _locs[1] = glGetUniformLocation(_program, "xplayer_TextureU");
        _locs[2] = glGetUniformLocation(_program, "xplayer_TextureV");
    }
        break;
    case XPLAYER_PIXEL_FORMAT_YUY2:
        succ = initShader("shaders/xplayer_yuy2.fs", vertex, _program);
        break;
    case XPLAYER_PIXEL_FORMAT_UYVY:
        succ = initShader("shaders/xplayer_uyvy.fs", vertex, _program);
        break;
    case XPLAYER_PIXEL_FORMAT_YVYU:
        succ = initShader("shaders/xplayer_yvyu.fs", vertex, _program);
        break;
    case XPLAYER_PIXEL_FORMAT_YUV420P10:
        succ = initShader("shaders/xplayer_yuv420p10.fs", vertex, _program);
        break;
    case XPLAYER_PIXEL_FORMAT_NV12:
    {
        succ = initShader("shaders/xplayer_nv12.fs", vertex, _program);
        _locs[0] = glGetUniformLocation(_program, "xplayer_TextureY");
        _locs[1] = glGetUniformLocation(_program, "xplayer_TextureUV");
    }
        break;
    case XPLAYER_PIXEL_FORMAT_NV21:
        succ = initShader("shaders/xplayer_nv21.fs", vertex, _program);
        break;
    case XPLAYER_PIXEL_FORMAT_P010:
    {
        succ = initShader("shaders/xplayer_p010.fs", vertex, _program);
        _locs[0] = glGetUniformLocation(_program, "xplayer_TextureY");
        _locs[1] = glGetUniformLocation(_program, "xplayer_TextureUV");
    }
        break;
    default:
        break;
    }

    if (succ)
        succ = initShader("shaders/xplayer_text.fs", vertex, _font_program, false);

    glDeleteShader(vertex);

    return succ;
}

void CXPlayerVideoRenderOpengl::uninitShader()
{
    if (0 != _program)
    {
        glDeleteProgram(_program);
        _program = 0;
    }

    if (0 != _font_program)
    {
        glDeleteProgram(_font_program);
        _font_program = 0;
    }

    memset(_locs, -1, sizeof(_locs));
}

bool CXPlayerVideoRenderOpengl::initShader(const std::string & path, GLuint vertex, GLuint & program, const bool & flag)
{
    bool succ = false;
    GLint status = 0;

    // 片段着色器
    GLuint fragment = 0;
    
    // 创建片段着色器
    fragment = compileShader(GL_FRAGMENT_SHADER, path);
    if (0 == fragment)
        goto end;

    if (!flag)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    // 链接着色器程序
    program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    if (flag)
    {
        glBindAttribLocation(program, _vertex_loc, "xplayer_Position");
        glBindAttribLocation(program, _texture_loc, "xplayer_TextureIn");
    }
    glLinkProgram(program);

    // 检查链接错误
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (GL_FALSE == status)
    {
        char buff[512] = {};
        glGetProgramInfoLog(program, 512, nullptr, buff);
        xpu_format_string(_err, "%s", buff);
        goto end;
    }

    succ = true;

end:
    if (0 != fragment)
    {
        glDeleteShader(fragment);
        fragment = 0;
    }
    if (0 != program && !succ)
    {
        glDeleteProgram(program);
        program = 0;
    }

    return succ;
}

bool CXPlayerVideoRenderOpengl::initTextures(const XPLAYER_PIXEL_FORMAT_TYPE & format, const bool & flag)
{
    auto found = _opts.find(format);
    if (_opts.end() == found)
    {
        xpu_format_string(_err, "Unsupported pixel format %d", format);
        return false;
    }

    int cnt = (*found).second.cnt;
    _textures.resize(cnt);
    glGenTextures(cnt, _textures.data());
    for (int i = 0; i < cnt; ++i)
    {
        glBindTexture(GL_TEXTURE_2D, _textures[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    if (flag && 0 != _font_texture)
        return true;

    glGenTextures(1, &_font_texture);
    glBindTexture(GL_TEXTURE_2D, _font_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}

void CXPlayerVideoRenderOpengl::uninitTextures(const bool & flag)
{
    for (auto & val : _textures)
    {
        glDeleteTextures(1, &val);
        val = 0;
    }
    _textures.clear();

    if (!flag && 0 != _font_texture)
    {
        glDeleteTextures(1, &_font_texture);
        _font_texture = 0;
    }
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

    glGenVertexArrays(1, &_vao);
    glGenBuffers(1, &_vbo);
    glGenBuffers(1, &_ebo);

    glBindVertexArray(_vao);

    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // 位置属性 (location = 0)
    glVertexAttribPointer(_vertex_loc, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(_vertex_loc);

    // 纹理坐标属性 (location = 1)
    glVertexAttribPointer(_texture_loc, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(_texture_loc);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void CXPlayerVideoRenderOpengl::uninitVertices()
{
    if (0 != _vao)
    {
        glDeleteVertexArrays(1, &_vao);
        _vao = 0;
    }

    if (0 != _vbo)
    {
        glDeleteBuffers(1, &_vbo);
        _vbo = 0;
    }

    if (0 != _ebo)
    {
        glDeleteBuffers(1, &_ebo);
        _ebo = 0;
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
    glGenVertexArrays(1, &_font_vao);
    glGenBuffers(1, &_font_vbo);
    glGenBuffers(1, &_font_ebo);

    glBindVertexArray(_font_vao);

    glBindBuffer(GL_ARRAY_BUFFER, _font_vbo);
    glBufferData(GL_ARRAY_BUFFER, 4 * 5 * sizeof(float), nullptr, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _font_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(_vertex_loc, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(_vertex_loc);

    glVertexAttribPointer(_texture_loc, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(_texture_loc);

    glBindVertexArray(0); // 解绑字体 VAO
}

void CXPlayerVideoRenderOpengl::uninitFontVertices()
{
    if (0 != _font_vao)
    {
        glDeleteVertexArrays(1, &_font_vao);
        _font_vao = 0;
    }

    if (0 != _font_vbo)
    {
        glDeleteBuffers(1, &_font_vbo);
        _font_vbo = 0;
    }

    if (0 != _font_ebo)
    {
        glDeleteBuffers(1, &_font_ebo);
        _font_ebo = 0;
    }
}

bool CXPlayerVideoRenderOpengl::cacheYUV420P(const int & index, uint8_t * data[8], int linesize[8])
{
    if (nullptr == data[0] || nullptr == data[1] || nullptr == data[2])
    {
        xpu_format_string(_err, "Invalid params");
        return false;
    }

    const int bytes = _cache[index]._width * _cache[index]._height;
    if (_cache[index]._data[0].size() != bytes)
    {
        _cache[index]._data[0].resize(bytes);
        _cache[index]._data[1].resize(bytes / 4);
        _cache[index]._data[2].resize(bytes / 4);
    }

    memcpy(_cache[index]._data[0].data(), data[0], bytes);
    memcpy(_cache[index]._data[1].data(), data[1], bytes / 4);
    memcpy(_cache[index]._data[2].data(), data[2], bytes / 4);

    return true;
}

bool CXPlayerVideoRenderOpengl::cacheYUY2(const int & index, uint8_t * data[8], int linesize[8])
{
    return true;
}

bool CXPlayerVideoRenderOpengl::cacheUYVY(const int & index, uint8_t * data[8], int linesize[8])
{
    return true;
}

bool CXPlayerVideoRenderOpengl::cacheYVYU(const int & index, uint8_t * data[8], int linesize[8])
{
    return true;
}

bool CXPlayerVideoRenderOpengl::cacheYUV420P10(const int & index, uint8_t * data[8], int linesize[8])
{
    if (nullptr == data[0] || nullptr == data[1] || nullptr == data[2])
    {
        xpu_format_string(_err, "Invalid params");
        return false;
    }

    const int bytes = _cache[index]._width * _cache[index]._height * 2;
    if (_cache[index]._data[0].size() != bytes)
    {
        _cache[index]._data[0].resize(bytes);
        _cache[index]._data[1].resize(bytes / 4);
        _cache[index]._data[2].resize(bytes / 4);
    }

    memcpy(_cache[index]._data[0].data(), data[0], bytes);
    memcpy(_cache[index]._data[1].data(), data[1], bytes / 4);
    memcpy(_cache[index]._data[2].data(), data[2], bytes / 4);

    return true;
}

bool CXPlayerVideoRenderOpengl::cacheNV12(const int & index, uint8_t * data[8], int linesize[8])
{
    if (nullptr == data[0] || nullptr == data[1])
    {
        xpu_format_string(_err, "Invalid params");
        return false;
    }

    const int bytes = _cache[index]._width * _cache[index]._height;
    if (_cache[index]._data[0].size() != bytes)
    {
        _cache[index]._data[0].resize(bytes);
        _cache[index]._data[1].resize(bytes / 2);
    }

    memcpy(_cache[index]._data[0].data(), data[0], bytes);
    memcpy(_cache[index]._data[1].data(), data[1], bytes / 2);

    return true;
}

bool CXPlayerVideoRenderOpengl::cacheNV21(const int & index, uint8_t * data[8], int linesize[8])
{
    return true;
}

bool CXPlayerVideoRenderOpengl::cacheP010(const int & index, uint8_t * data[8], int linesize[8])
{
    if (nullptr == data[0] || nullptr == data[1])
    {
        xpu_format_string(_err, "Invalid params");
        return false;
    }

    const int bytes = _cache[index]._width * _cache[index]._height * 2;
    if (_cache[index]._data[0].size() != bytes)
    {
        _cache[index]._data[0].resize(bytes);
        _cache[index]._data[1].resize(bytes / 2);
    }

    memcpy(_cache[index]._data[0].data(), data[0], bytes);
    memcpy(_cache[index]._data[1].data(), data[1], bytes / 2);

    return true;
}

void CXPlayerVideoRenderOpengl::renderYUV420P(const int & w, const int & h, const int & index)
{
    glUniform1i(glGetUniformLocation(_program, "xplayer_ColorSpace"), 0);

    // 2. 绑定 VAO
    glBindVertexArray(_vao);

    // 更新 Y 纹理
    glActiveTexture(GL_TEXTURE0); // 激活纹理单元 0
    glBindTexture(GL_TEXTURE_2D, _textures[0]);
    if (_changed.load())
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, _cache[index]._data[0].constData());
    else
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RED, GL_UNSIGNED_BYTE, _cache[index]._data[0].constData());
    // 将纹理单元 0 绑定到着色器中的 uniform sampler2D textureY
    glUniform1i(_locs[0], 0);

    // 更新 U 纹理
    glActiveTexture(GL_TEXTURE1); // 激活纹理单元 1
    glBindTexture(GL_TEXTURE_2D, _textures[1]);
    if (_changed.load())
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w / 2, h / 2, 0, GL_RED, GL_UNSIGNED_BYTE, _cache[index]._data[1].constData());
    else
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w / 2, h / 2, GL_RED, GL_UNSIGNED_BYTE, _cache[index]._data[1].constData());
    glUniform1i(_locs[1], 1);

    // 更新 V 纹理
    glActiveTexture(GL_TEXTURE2); // 激活纹理单元 2
    glBindTexture(GL_TEXTURE_2D, _textures[2]);
    if (_changed.load())
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w / 2, h / 2, 0, GL_RED, GL_UNSIGNED_BYTE, _cache[index]._data[2].constData());
    else
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w / 2, h / 2, GL_RED, GL_UNSIGNED_BYTE, _cache[index]._data[2].constData());
    glUniform1i(_locs[2], 2);

    // 3. 绘制
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void CXPlayerVideoRenderOpengl::renderYUY2(const int & w, const int & h, const int & index)
{
}

void CXPlayerVideoRenderOpengl::renderUYVY(const int & w, const int & h, const int & index)
{
}

void CXPlayerVideoRenderOpengl::renderYVYU(const int & w, const int & h, const int & index)
{
}

void CXPlayerVideoRenderOpengl::renderYUV420P10(const int & w, const int & h, const int & index)
{
}

void CXPlayerVideoRenderOpengl::renderNV12(const int & w, const int & h, const int & index)
{
    // 2. 绑定 VAO
    glBindVertexArray(_vao);

    // 更新 Y 纹理
    glActiveTexture(GL_TEXTURE0); // 激活纹理单元 0
    glBindTexture(GL_TEXTURE_2D, _textures[0]);
    if (_changed.load())
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, _cache[index]._data[0].constData());
    else
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RED, GL_UNSIGNED_BYTE, _cache[index]._data[0].constData());
    // 将纹理单元 0 绑定到着色器中的 uniform sampler2D textureY
    glUniform1i(_locs[0], 0);

    // 更新 UV 纹理
    glActiveTexture(GL_TEXTURE1); // 激活纹理单元 1
    glBindTexture(GL_TEXTURE_2D, _textures[1]);
    if (_changed.load())
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, w / 2, h / 2, 0, GL_RG, GL_UNSIGNED_BYTE, _cache[index]._data[1].constData());
    else
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w / 2, h / 2, GL_RG, GL_UNSIGNED_BYTE, _cache[index]._data[1].constData());
    glUniform1i(_locs[1], 1);

    // 3. 绘制
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void CXPlayerVideoRenderOpengl::renderNV21(const int & w, const int & h, const int & index)
{
}

void CXPlayerVideoRenderOpengl::renderP010(const int & w, const int & h, const int & index)
{
    // 2. 绑定 VAO
    glBindVertexArray(_vao);

    // 更新 Y 纹理
    glActiveTexture(GL_TEXTURE0); // 激活纹理单元 0
    glBindTexture(GL_TEXTURE_2D, _textures[0]);
    if (_changed.load())
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, w, h, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, _cache[index]._data[0].constData());
    else
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, _cache[index]._data[0].constData());
    // 将纹理单元 0 绑定到着色器中的 uniform sampler2D textureY
    glUniform1i(_locs[0], 0);

    // 更新 UV 纹理
    glActiveTexture(GL_TEXTURE1); // 激活纹理单元 1
    glBindTexture(GL_TEXTURE_2D, _textures[1]);
    if (_changed.load())
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w / 2, h / 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, _cache[index]._data[1].constData());
    else
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w / 2, h / 2, GL_RGBA, GL_UNSIGNED_BYTE, _cache[index]._data[1].constData());
    glUniform1i(_locs[1], 1);

    // 3. 绘制
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

bool CXPlayerVideoRenderOpengl::rendererText(const std::string & str)
{
    if (str.empty() || nullptr == _font_ctx)
        return true;

    glUseProgram(_font_program);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, _font_texture);
    glUniform1i(glGetUniformLocation(_font_program, "xplayer_TextureStr"), 3);
    glUniform4f(glGetUniformLocation(_font_program, "xplayer_FontColor"), _color.x(), _color.y(), _color.z(), _color.w());

    glBindVertexArray(_font_vao);

    // 行距
    const float line_space = 12.0f;
    // 边距
    const float margin = 10.0f;
    const auto screen_ratio_x = 2.0f / static_cast<float>(this->width());
    const auto screen_ratio_y = 2.0f / static_cast<float>(this->height());
    float cur_height = margin;

    TTF_SetFontSize(_font_ctx, _font_size);

    SDL_Color font_color = { 0 };
    font_color.r = static_cast<uint8_t>(_font_color.red);
    font_color.g = static_cast<uint8_t>(_font_color.green);
    font_color.b = static_cast<uint8_t>(_font_color.blue);
    font_color.a = static_cast<uint8_t>(_font_color.alpha);

    std::string val;
    std::istringstream iss(str);
    while (getline(iss, val, '\n'))
    {
        if (val.empty())
            continue;

        SDL_Surface * surface = TTF_RenderUTF8_Blended(_font_ctx, val.c_str(), font_color);
        if (nullptr == surface)
        {
            xpu_format_string(_err, "TTF_RenderUTF8_Blended error: %s", TTF_GetError());
            return false;
        }

        const auto width = surface->w;
        const auto height = surface->h;

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // 允许非4字节对齐
        glPixelStorei(GL_UNPACK_ROW_LENGTH, surface->pitch / 4); // 告诉 OpenGL 每行有多少像素

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, surface->pixels);

        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

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
        glBindBuffer(GL_ARRAY_BUFFER, _font_vbo);
        // 使用 glBufferSubData 比 glBufferData 更安全，不需要重新分配内存
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        cur_height += (line_space + height);
    }

    glBindVertexArray(0);
    glUseProgram(0);

    return true;
}
