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

    void mousePressEvent(QMouseEvent * event) override;   // 鼠标点击
    void mouseMoveEvent(QMouseEvent * event) override;    // 鼠标移动
    void mouseReleaseEvent(QMouseEvent * event) override; // 鼠标释放
    void resizeEvent(QResizeEvent * event) override;
    void timerEvent(QTimerEvent * event) override;
    bool eventFilter(QObject * obj, QEvent * event) override;
    void dragEnterEvent(QDragEnterEvent * event) override;
    void dropEvent(QDropEvent * event) override;

private slots:
    void onVolumeButtonEnter();
    void onVolumeChanged(int vol);

private:
    void play(const std::string & url);

private:
    Ui::XPlayerClass ui;

    CVolumeWidget * m_widgetVolume = nullptr;
    QTimer * m_tmVolume = nullptr;

    QPoint m_ptStart;
    bool m_blPressed = false;
    bool m_blResize = false;
    int m_iEdge = 0;
    QRect m_rectStart;
    int m_iTid = -1;

    static const int BORDER_WIDTH = 10;

    QMenu * m_pclsChoices = nullptr;
    QMenu * m_pmnuVideo = nullptr;
    QMenu * m_pmnuAudio = nullptr;
};
