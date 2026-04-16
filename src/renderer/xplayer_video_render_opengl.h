#ifndef __XPLAYER_VIDEO_RENDER_OPENGL_H__
#define __XPLAYER_VIDEO_RENDER_OPENGL_H__

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_0>
#include <QOpenGLShaderProgram>
#include <QOpenGLContext>

#define XPLAYER_OPENGL_FRAME_CACHE 3

class CXPlayerVideoRenderOpengl : public QOpenGLWidget, protected QOpenGLFunctions_3_0
{
	Q_OBJECT
public:
    explicit CXPlayerVideoRenderOpengl(QWidget * parent = nullptr);
	~CXPlayerVideoRenderOpengl();

    // 改变画面大小
    bool resizeImage(int width, int height);

    // 渲染
    bool renderer(uint8_t * data[8], const std::string & str = "");

    // 错误信息
    const char * err() const;

signals:
    void frameReady(); // 定义一个信号，用于通知主线程

protected:
    // 1. 初始化 OpenGL 资源和状态（只调用一次）
    void initializeGL() override;

    // 2. 渲染 OpenGL 场景（循环调用）
    void paintGL() override;

    // 3. 处理窗口大小改变（调整视口等）
    void resizeGL(int w, int h) override;

private:
    GLuint complileShader(GLenum type, const char * src);

    bool initShader();

    bool initTextures();

    void initVertices();

private:
    // 大小发生改变
    std::atomic_bool m_blChanged = { false };
    // 宽度
    std::atomic_int m_iWidth = { 0 };
    // 高度
    std::atomic_int m_iHeight = { 0 };

    // 纹理器
    GLuint m_uiTexures[3] = {};
    //
    GLuint m_uiProgram = 0;

    // 写
    std::atomic_int m_iWriteIndex = { 0 };
    // 读
    std::atomic_int m_iReadIndex = { 0 };
    // 是否有新帧
    std::atomic_bool m_blExist = { false };

    GLuint m_uiVertexLocation = 0;
    GLuint m_uiTextureLocation = 1;

    GLuint m_uiVAO = 0;
    GLuint m_uiVBO = 0;
    GLuint m_uiEBO = 0;

    QByteArray m_ucCache[XPLAYER_OPENGL_FRAME_CACHE][3];

    // 错误信息
    std::string _err;
};

#endif
