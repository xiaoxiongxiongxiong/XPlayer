#ifndef __XPLAYER_VIDEO_RENDER_OPENGL_H__
#define __XPLAYER_VIDEO_RENDER_OPENGL_H__

#include "xplayer_video_renderer.h"
#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_0>
#include <QOpenGLShaderProgram>
#include <QOpenGLContext>
#include <QVector4D>

#define XPLAYER_OPENGL_FRAME_CACHE 3

typedef struct _xplayer_opengl_option_t
{
    int cnt;  // texture个数
    std::function<bool(const int &, uint8_t * data[8], int linesize[8])> cache;
    std::function<void(const int &, const int &, const int &)> render;
} xplayer_opengl_option_t;

class CXPlayerOpenGLCache
{
public:
    int _width = 0;
    int _height = 0;
    XPLAYER_PIXEL_FORMAT_TYPE _format = XPLAYER_PIXEL_FORMAT_NONE;
    QByteArray _data[4];
};

class CXPlayerVideoRenderOpengl : public QOpenGLWidget, protected QOpenGLFunctions_3_0, public ICXPlayerVideoRenderer
{
	Q_OBJECT
public:
    explicit CXPlayerVideoRenderOpengl(QWidget * parent = nullptr);
	~CXPlayerVideoRenderOpengl();

    // 是否支持对应像素格式
    bool supportedPixelFormat(std::vector<XPLAYER_PIXEL_FORMAT_TYPE> & formats) override;

    // 设置字体路径
    void setFontPath(const std::string & path) override;
    // 设置字体大小
    void setFontSize(int size) override;
    // 设置字体颜色
    void setFontColor(const xplayer_color_t & color) override;

    // 创建
    bool create(const void * wnd, int width, int height) override;
    // 销毁
    void destroy() override;

    // 改变窗口大小
    bool resize(int width, int height) override;

    // 渲染
    bool renderer(int width, int height, XPLAYER_PIXEL_FORMAT_TYPE format, 
                  uint8_t * data[8], int linesize[8], const std::string & str = "") override;

    // 清空画面
    void clear() override;

    // 获取渲染器类型
    XPLAYER_VIDEO_RENDERER_TYPE getType() const override;

    // 错误信息
    const char * err() const override;

protected:
    // 1. 初始化 OpenGL 资源和状态（只调用一次）
    void initializeGL() override;

    // 2. 渲染 OpenGL 场景（循环调用）
    void paintGL() override;

    // 3. 处理窗口大小改变（调整视口等）
    void resizeGL(int w, int h) override;

private:
    // 初始化缓存处理
    void initOptions();

    GLuint compileShader(GLenum type, const std::string & path);

    bool initShader(const XPLAYER_PIXEL_FORMAT_TYPE & format);
    void uninitShader();

    bool initShader(const std::string & path, GLuint vertex, GLuint & program, const bool & flag = true);

    bool initTextures(const XPLAYER_PIXEL_FORMAT_TYPE & format, const bool & flag = false);
    void uninitTextures(const bool & flag = false);

    void initVertices();
    void uninitVertices();

    void initFontVertices();
    void uninitFontVertices();

    // 保存图像数据
    bool cacheYUV420P(const int & index, uint8_t * data[8], int linesize[8]);
    bool cacheYUY2(const int & index, uint8_t * data[8], int linesize[8]);
    bool cacheUYVY(const int & index, uint8_t * data[8], int linesize[8]);
    bool cacheYVYU(const int & index, uint8_t * data[8], int linesize[8]);
    bool cacheYUV420P10(const int & index, uint8_t * data[8], int linesize[8]);
    bool cacheNV12(const int & index, uint8_t * data[8], int linesize[8]);
    bool cacheNV21(const int & index, uint8_t * data[8], int linesize[8]);
    bool cacheP010(const int & index, uint8_t * data[8], int linesize[8]);

    // 渲染yuv420p
    void renderYUV420P(const int & w, const int & h, const int & index);
    void renderYUY2(const int & w, const int & h, const int & index);
    void renderUYVY(const int & w, const int & h, const int & index);
    void renderYVYU(const int & w, const int & h, const int & index);
    void renderYUV420P10(const int & w, const int & h, const int & index);
    void renderNV12(const int & w, const int & h, const int & index);
    void renderNV21(const int & w, const int & h, const int & index);
    void renderP010(const int & w, const int & h, const int & index);

    // 渲染字体
    bool rendererText(const std::string & str);

private:
    std::unordered_map<XPLAYER_PIXEL_FORMAT_TYPE, xplayer_opengl_option_t> _opts;

    // 缓冲
    CXPlayerOpenGLCache _cache[XPLAYER_OPENGL_FRAME_CACHE];
    // 写索引
    std::atomic_int _write_index = { 0 };
    // 读索引
    std::atomic_int _read_index = { 0 };
    // 写次数
    std::atomic_int _write_times = { 0 };
    // 读次数
    std::atomic_int _read_times = { 0 };

    GLuint _vertex_loc = 0;
    GLuint _texture_loc = 1;

    // 图像
    std::vector<GLuint> _textures;
    GLuint _program = 0;
    GLuint _vao = 0;
    GLuint _vbo = 0;
    GLuint _ebo = 0;
    GLint _locs[3] = { -1 };

    // 字体
    GLuint _font_texture = 0;
    GLuint _font_program = 0;
    GLuint _font_vao = 0;
    GLuint _font_vbo = 0;
    GLuint _font_ebo = 0;
    QVector4D _color;

    // 错误信息
    std::string _err;
};

#endif
