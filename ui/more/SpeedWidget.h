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

public slots:
    void onRadioButtonToggled(int id, bool checked);
    void onSliderValueChanged(int value);

protected:
    void paintEvent(QPaintEvent * event) override;
    bool eventFilter(QObject * obj, QEvent * event) override;

private:
    Ui::CSpeedWidget ui;

    QButtonGroup * m_grpSpeed = nullptr;
};

#endif // SPEEDWIDGET_H
