#include "xplayer_video_render_opengl.h"

CXPlayerVideoRenderOpengl::CXPlayerVideoRenderOpengl(QWidget * parent)
    : QOpenGLWidget(parent)
{
    setUpdateBehavior(QOpenGLWidget::PartialUpdate);
    // 构造函数中通常不执行 OpenGL 操作，因为此时上下文可能还未准备好
}

CXPlayerVideoRenderOpengl::~CXPlayerVideoRenderOpengl()
{
    // 清理资源（如果有手动分配的显存或对象）
    // makeCurrent() 会被自动调用，所以这里可以安全地删除 OpenGL 对象
}

// ---------------------------------------------------------
// 初始化：在渲染前调用一次
// ---------------------------------------------------------
void CXPlayerVideoRenderOpengl::initializeGL()
{
    // 【重要】初始化 OpenGL 函数解析器
    // 如果不写这一行，调用 glClear 等函数时会崩溃或无效
    initializeOpenGLFunctions();

    // 在这里设置 OpenGL 状态
    // 例如：开启深度测试、加载着色器、创建纹理等
    //glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // 设置清屏颜色（深灰色）

    // 如果是混合绘制（使用 QPainter），可以设置背景透明
    // setFormat(QSurfaceFormat::defaultFormat()); 
}

// ---------------------------------------------------------
// 渲染：每次需要重绘时调用
// ---------------------------------------------------------
void CXPlayerVideoRenderOpengl::paintGL()
{
    // 1. 清除颜色缓冲和深度缓冲
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 2. 在这里写你的 OpenGL 渲染代码
    // -----------------------------------------------------
    // 示例：如果你是用 SDL2 渲染，这里通常是空的，
    // 因为 SDL2 会直接操作上下文。
    // 但如果你是用 Qt 的 OpenGL 接口绘图，代码如下：
    // -----------------------------------------------------

    // 比如：绘制一个简单的图形或绑定纹理
    // glDrawArrays(...);

    // 如果你只是想让背景显示 initializeGL 设置的颜色，
    // 那么除了 glClear 什么都不用写。
}

// ---------------------------------------------------------
// 大小调整：窗口尺寸改变时调用
// ---------------------------------------------------------
void CXPlayerVideoRenderOpengl::resizeGL(int w, int h)
{
    // 设置视口，确保 OpenGL 渲染区域与控件大小一致
    glViewport(0, 0, w, h);

    // 如果有 3D 投影矩阵，需要在这里更新投影矩阵的宽高比
}


