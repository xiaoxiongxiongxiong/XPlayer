#pragma once

#include <QWidget>
#include <QTimer>
#include <QPaintEvent>
#include "ui_SliderWidget.h"

class CSliderWidget : public QWidget
{
    Q_OBJECT

public:
    CSliderWidget(QWidget *parent = nullptr);
    ~CSliderWidget();

    int getValue() const;
    void setValue(int vol);

    void showSlider(const QPoint & pos);
    void hideSlider();

signals:
    void valueChanged(int volume);

private slots:
    void onSliderValueChanged(int value);
    void hideWidget();

protected:
    void paintEvent(QPaintEvent * event) override;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEnterEvent * event) override;
#else
    void enterEvent(QEvent * event) override;
#endif
    void leaveEvent(QEvent * event) override;

private:
    Ui::CSliderWidgetClass ui;

    QTimer * m_ptrTimer = nullptr;
};

