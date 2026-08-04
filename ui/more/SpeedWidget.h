#ifndef SPEEDWIDGET_H
#define SPEEDWIDGET_H

#include <QWidget>
#include "ui_SpeedWidget.h"

class CSpeedWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CSpeedWidget(QWidget *parent = nullptr);
    ~CSpeedWidget();

protected:
    void paintEvent(QPaintEvent * event) override;

private:
    Ui::CSpeedWidget ui;
};

#endif // SPEEDWIDGET_H
