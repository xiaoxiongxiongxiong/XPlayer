#ifndef __XPLAYER_VIDEO_RENDER_H__
#define __XPLAYER_VIDEO_RENDER_H__

#include <QOpenglWidget>
#include <QOpenGLFunctions_3_0>
#include <QPainter>

class CXPlayerVideoRender :public QOpenGLWidget, protected QOpenGLFunctions_3_0
{
    Q_OBJECT
public:
    CXPlayerVideoRender(QWidget * parent = nullptr);
    ~CXPlayerVideoRender() = default;

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

    void paintEvent(QPaintEvent * event) override;
};

#endif
