#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_XPlayer.h"

class QMenuBar;

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
    void onBtnClickedVod();
    void onBtnClickedLive();
    void onBtnClickedCtrl();
    void onBtnClickedStop();
    void onBtnClickedBackward();
    void onBtnClickedForward();
    void onBtnClickedLast();
    void onBtnClickedNext();

protected:
    void paintEvent(QPaintEvent * event) override;
    void resizeEvent(QResizeEvent * event) override;

private:
    Ui::XPlayerClass ui;

    QPoint m_lastPos;
    bool m_blPressed = false;
};
