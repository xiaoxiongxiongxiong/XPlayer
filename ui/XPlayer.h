#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_XPlayer.h"

class QMenuBar;
class CVolumeWidget;

class XPlayer : public QMainWindow
{
    Q_OBJECT

public:
    XPlayer(QWidget * parent = nullptr);
    ~XPlayer();

    void mousePressEvent(QMouseEvent * event) override;   // 鼠标点击
    void mouseMoveEvent(QMouseEvent * event) override;    // 鼠标移动
    void mouseReleaseEvent(QMouseEvent * event) override; // 鼠标释放

public slots:
    void onBtnClickedMinimize();
    void onBtnClickedMaximize();
    void onBtnClickedClose();

    void onBtnClickedVolume();
    void onBtnClickedVod();
    void onBtnClickedLive();
    void onBtnClickedCtrl();
    void onBtnClickedStop();
    void onBtnClickedBackward();
    void onBtnClickedForward();
    void onBtnClickedLast();
    void onBtnClickedNext();
    void onBtnClickedRecord();

protected:
    void paintEvent(QPaintEvent * event) override;
    bool eventFilter(QObject * obj, QEvent * event) override;

private slots:
    void onVolumeButtonEnter();
    void onVolumeButtonLeave();
    void onVolumeChanged(int vol);

private:
    void play(const std::string & url);

private:
    Ui::XPlayerClass ui;

    CVolumeWidget * m_widgetVolume = nullptr;
    QTimer * m_tmVolume = nullptr;

    QPoint m_lastPos;
    bool m_blPressed = false;
    QMenu * m_pclsChoices = nullptr;
};
