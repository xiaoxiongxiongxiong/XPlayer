#include "SliderWidget.h"
#include <QPainter>

CSliderWidget::CSliderWidget(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);

    setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    ui.m_sldSlider->setRange(0, 128);
    ui.m_sldSlider->setTickPosition(QSlider::TicksBothSides);

    m_ptrTimer = new QTimer(this);
    m_ptrTimer->setSingleShot(true);
    m_ptrTimer->setInterval(300); // 延迟300ms隐藏
    connect(m_ptrTimer, &QTimer::timeout, this, &CSliderWidget::hideWidget);
    connect(ui.m_sldSlider, &QSlider::valueChanged, this, &CSliderWidget::onSliderValueChanged);
}

CSliderWidget::~CSliderWidget()
{
    if (nullptr != m_ptrTimer)
    {
        delete m_ptrTimer;
        m_ptrTimer = nullptr;
    }
}

void CSliderWidget::showSlider(const QPoint & pos)
{
    move(pos);
    show();
    raise();
    activateWindow();
    m_ptrTimer->stop(); // 显示时停止隐藏计时器
}

int CSliderWidget::getValue() const
{
    return ui.m_sldSlider->value();
}

void CSliderWidget::setValue(int vol)
{
    ui.m_sldSlider->setValue(vol);
}

void CSliderWidget::hideSlider()
{
    if (isVisible())
        m_ptrTimer->start();  // 启动定时器
}

void CSliderWidget::onSliderValueChanged(int value)
{
    emit valueChanged(value);
}

void CSliderWidget::paintEvent(QPaintEvent * event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 气泡背景：圆角矩形 + 半透明
    QRect rect = this->rect().adjusted(1, 1, -1, -1);
    QColor bgColor(255, 255, 255, 180); // 深灰半透明
    painter.setBrush(bgColor);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect, 6, 6); // 圆角 8px
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void CSliderWidget::enterEvent(QEnterEvent * event)
#else
void CSliderWidget::enterEvent(QEvent * event)
#endif
{
    m_ptrTimer->stop(); // 鼠标进入，取消隐藏
    QWidget::enterEvent(event);
}

void CSliderWidget::leaveEvent(QEvent * event)
{
    m_ptrTimer->start(); // 鼠标离开，启动隐藏计时器
    QWidget::leaveEvent(event);
}

void CSliderWidget::hideWidget()
{
    hide();
}
