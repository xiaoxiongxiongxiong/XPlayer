#ifndef __XPLAYER_VIDEO_RENDER_OPENGL_H__
#define __XPLAYER_VIDEO_RENDER_OPENGL_H__

#include "xplayer_video_renderer.h"
#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_0>
#include <QOpenGLShaderProgram>
#include <QOpenGLContext>

#define XPLAYER_OPENGL_FRAME_CACHE 3

class CXPlayerVideoRenderOpengl : public QOpenGLWidget, protected QOpenGLFunctions_3_0, public ICXPlayerVideoRenderer
{
	Q_OBJECT
public:
    explicit CXPlayerVideoRenderOpengl(QWidget * parent = nullptr);
	~CXPlayerVideoRenderOpengl();

    // 是否支持对应像素格式
    bool supportedPixelFormat(std::vector<XPLAYER_PIXEL_FORMAT_TYPE> & formats) override;

    // 创建
    bool create(const void * wnd, int width, int height, const std::string & path, const int & size) override;
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
    GLuint compileShader(GLenum type, const std::string & path);

    bool initShader(const XPLAYER_PIXEL_FORMAT_TYPE & format);
    bool initShaderYUV420P(GLuint vertex);
    bool initShaderYUY2(GLuint vertex);
    bool initShaderUYVY(GLuint vertex);
    bool initShaderYVYU(GLuint vertex);
    bool initShaderYUV420P10(GLuint vertex);
    bool initShaderNV12(GLuint vertex);
    bool initShaderNV21(GLuint vertex);
    bool initShaderP010(GLuint vertex);

    // 初始化字体shader
    bool initFontShader(GLuint vertex);

    bool initTextures();

    void initVertices();
    void uninitVertices();

    void initFontVertices();
    void uninitFontVertices();

    // 渲染yuv420p
    void renderYUV420P(const int & w, const int & h, const int & index);
    void renderYUY2(const int & w, const int & h, const int & index);
    void renderUYVY(const int & w, const int & h, const int & index);
    void renderYVYU(const int & w, const int & h, const int & index);
    void renderYUV420P10(const int & w, const int & h, const int & index);
    void renderNV12(const int & w, const int & h, const int & index);
    void renderNV21(const int & w, const int & h, const int & index);
    void renderP010(const int & w, const int & h, const int & index);

    // 渲染
    bool rendererText(const std::string & str);

private:
    // 纹理器 0~2 yuv 3-text
    GLuint m_uiTexures[4] = {};
    // YUV
    GLuint m_uiProgram = 0;
    // 字体
    GLuint m_uiFontProgram = 0;
    // yuv Location
    GLint m_iYUVLocation[3] = { 0 };

    // 缓冲
    QByteArray m_ucCache[XPLAYER_OPENGL_FRAME_CACHE][4];
    // 写索引
    std::atomic_int m_iWriteIndex = { 0 };
    // 读索引
    std::atomic_int m_iReadIndex = { 0 };
    // 写次数
    std::atomic_int m_iWriteTimes = { 0 };
    // 读次数
    std::atomic_int m_iReadTimes = { 0 };

    GLuint m_uiVertexLocation = 0;
    GLuint m_uiTextureLocation = 1;

    GLuint m_uiVAO = 0;
    GLuint m_uiVBO = 0;
    GLuint m_uiEBO = 0;

    GLuint m_uiFontVAO = 0;
    GLuint m_uiFontVBO = 0;
    GLuint m_uiFontEBO = 0;

    // 错误信息
    std::string _err;
};

#endif
