#ifndef __XPLAYER_VIDEO_RENDER_OPENGL_H__
#define __XPLAYER_VIDEO_RENDER_OPENGL_H__

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_0>
#include <QOpenGLShaderProgram>
#include <QOpenGLContext>

class CXPlayerVideoRenderOpengl : public QOpenGLWidget, protected QOpenGLFunctions_3_0
{
	Q_OBJECT
public:
    explicit CXPlayerVideoRenderOpengl(QWidget * parent = nullptr);
	~CXPlayerVideoRenderOpengl();

    // 设置尺寸
    void setSize(int width, int height);

    // 渲染
    void renderer(const uint8_t * y, const uint8_t * u, const uint8_t * v, const std::string & str = "");

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
    // 纹理器
    GLuint m_uiTexures[3] = {};
    //
    GLuint m_uiProgram = 0;

    // 宽度
    std::atomic_int m_iWidth = { 0 };
    // 高度
    std::atomic_int m_iHeight = { 0 };

    GLuint m_uiVertexLocation = 0;
    GLuint m_uiTextureLocation = 1;

    GLuint m_uiVAO = 0;
    GLuint m_uiVBO = 0;

    // 数据缓存
    QByteArray m_yData;
    QByteArray m_uData;
    QByteArray m_vData;

    // 错误信息
    std::string _err;
};

#endif
