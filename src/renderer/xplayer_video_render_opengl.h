#ifndef __XPLAYER_VIDEO_RENDER_OPENGL_H__
#define __XPLAYER_VIDEO_RENDER_OPENGL_H__

#include <QOpenGLWidget>
#include <QOpenGLFunctions>

class CXPlayerVideoRenderOpengl : public QOpenGLWidget, protected QOpenGLFunctions
{
	Q_OBJECT
public:
    explicit CXPlayerVideoRenderOpengl(QWidget * parent = nullptr);
	~CXPlayerVideoRenderOpengl();

protected:
    // 1. 初始化 OpenGL 资源和状态（只调用一次）
    void initializeGL() override;

    // 2. 渲染 OpenGL 场景（循环调用）
    void paintGL() override;

    // 3. 处理窗口大小改变（调整视口等）
    void resizeGL(int w, int h) override;

};

#endif
