#pragma once

#include <FramelessHelper/Widgets/framelesshelperwidgets_global.h>
#include <FramelessHelper/Widgets/framelessmainwindow.h>

#include <QtWidgets/QMainWindow>
#include "ui_XPlayer.h"

class QMenuBar;
class CVolumeWidget;
class CXPlayerRecord;

class XPlayer : public FRAMELESSHELPER_NAMESPACE::FramelessMainWindow
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
    void onLstDbclickedRecord(QListWidgetItem * item);

private:
    void play(const std::string & url);
    // 加载播放记录
    bool loadPlayRecord();
    // 卸载播放记录
    void unloadPlayRecord();

private:
    Ui::XPlayerClass ui;

    CVolumeWidget * m_widgetVolume = nullptr;
    QTimer * m_tmVolume = nullptr;

    QPoint m_ptStart;
    bool m_blPressed = false;
    int m_iTid = -1;

    QMenu * m_pclsChoices = nullptr;
    QMenu * m_pmnuVideo = nullptr;
    QMenu * m_pmnuAudio = nullptr;

    CXPlayerRecord * m_pclsVod = nullptr;
    CXPlayerRecord * m_pclsLive = nullptr;
};
