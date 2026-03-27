#pragma once

#include <QWidget>
#include <QTimer>
#include <QPaintEvent>
#include "ui_CVolumeWidget.h"

class CVolumeWidget : public QWidget
{
    Q_OBJECT

public:
    CVolumeWidget(QWidget *parent = nullptr);
    ~CVolumeWidget();

    int getVolume() const;
    void setVolume(int vol);

    void showVolume(const QPoint & pos);
    void hideVolume();

signals:
    void volumeChanged(int volume);

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
    Ui::CVolumeWidgetClass ui;

    QTimer * m_tmVolume = nullptr;
};

