#include "VolumeWidget.h"
#include <QPainter>

CVolumeWidget::CVolumeWidget(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);

    setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    ui.m_sldVolume->setRange(0, 128);
    ui.m_sldVolume->setTickPosition(QSlider::TicksBothSides);

    m_tmVolume = new QTimer(this);
    m_tmVolume->setSingleShot(true);
    m_tmVolume->setInterval(300); // 延迟300ms隐藏
    connect(m_tmVolume, &QTimer::timeout, this, &CVolumeWidget::hideWidget);
    connect(ui.m_sldVolume, &QSlider::valueChanged, this, &CVolumeWidget::onSliderValueChanged);
}

CVolumeWidget::~CVolumeWidget()
{
    if (nullptr != m_tmVolume)
    {
        delete m_tmVolume;
        m_tmVolume = nullptr;
    }
}

void CVolumeWidget::showVolume(const QPoint & pos)
{
    move(pos);
    show();
    raise();
    activateWindow();
    m_tmVolume->stop(); // 显示时停止隐藏计时器
}

int CVolumeWidget::getVolume() const
{
    return ui.m_sldVolume->value();
}

void CVolumeWidget::setVolume(int vol)
{
    ui.m_sldVolume->setValue(vol);
}

void CVolumeWidget::hideVolume()
{
    if (isVisible())
        m_tmVolume->start();  // 启动定时器
}

void CVolumeWidget::onSliderValueChanged(int value)
{
    emit volumeChanged(value);
}

void CVolumeWidget::paintEvent(QPaintEvent * event)
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
void CVolumeWidget::enterEvent(QEnterEvent * event)
#else
void CVolumeWidget::enterEvent(QEvent * event)
#endif
{
    m_tmVolume->stop(); // 鼠标进入，取消隐藏
    QWidget::enterEvent(event);
}

void CVolumeWidget::leaveEvent(QEvent * event)
{
    m_tmVolume->start(); // 鼠标离开，启动隐藏计时器
    QWidget::leaveEvent(event);
}

void CVolumeWidget::hideWidget()
{
    hide();
}
