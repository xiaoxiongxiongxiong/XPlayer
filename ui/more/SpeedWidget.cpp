#include "SpeedWidget.h"

#include <QPainter>

CSpeedWidget::CSpeedWidget(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);

    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);

    setAttribute(Qt::WA_TranslucentBackground);
}

CSpeedWidget::~CSpeedWidget()
{
}

void CSpeedWidget::paintEvent(QPaintEvent * event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPainterPath path;
    path.addRoundedRect(rect(), 12, 12);

    painter.fillPath(path, Qt::white);

    painter.setClipPath(path);

    QWidget::paintEvent(event);
}
