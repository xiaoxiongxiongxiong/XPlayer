#ifndef __XPLAYER_VIDEO_RENDER_H__
#define __XPLAYER_VIDEO_RENDER_H__

#include <QOpenglWidget>
#include <QOpenGLFunctions_3_0>

class CVideoRender :public QOpenGLWidget, protected QOpenGLFunctions_3_0
{
    Q_OBJECT
public:
    CVideoRender(QWidget * parent = nullptr);
    ~CVideoRender() = default;

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;
};

#endif
