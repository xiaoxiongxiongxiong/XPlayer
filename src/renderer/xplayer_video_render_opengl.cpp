#include "xplayer_video_render_opengl.h"

CXPlayerVideoRender::CXPlayerVideoRender(QWidget * parent) :
    QOpenGLWidget(parent)
{

}

void CXPlayerVideoRender::initializeGL()
{

}

void CXPlayerVideoRender::paintGL()
{
}

void CXPlayerVideoRender::resizeGL(int w, int h)
{

}

void CXPlayerVideoRender::paintEvent(QPaintEvent * event)
{
    //QPainter painter(this);
    //painter.setPen(Qt::NoPen); // 关闭默认的绘图边框模式
    //painter.setBrush(Qt::white); // 设置底色为黑色
    //painter.drawRect(0, 0, width() - 1, height() - 1); // 绘制矩形，除最后一个像素

    //// 绘制底边框
    //painter.setPen(QPen(Qt::white, 2)); // 设置蓝色边框，宽度为2
    //painter.drawLine(0, height() - 2, width(), height() - 2); // 绘制底边框

    //// 调用基类函数绘制子部件
    //QOpenGLWidget::paintEvent(event);
}
