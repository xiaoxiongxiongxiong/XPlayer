#pragma once

#include <FramelessHelper/Widgets/framelesshelperwidgets_global.h>
#include <FramelessHelper/Widgets/framelessmainwindow.h>

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QListWidget>
#include "ui_XPlayer.h"

class QMenuBar;
class CVolumeWidget;
class CRecordWidget;

class XPlayer : public FRAMELESSHELPER_NAMESPACE::FramelessMainWindow
{
    Q_OBJECT

public:
    XPlayer(QWidget * parent = nullptr);
    ~XPlayer();

protected:
    void keyPressEvent(QKeyEvent * event) override;
    void keyReleaseEvent(QKeyEvent * event) override;
    void mousePressEvent(QMouseEvent * event) override;   // 鼠标点击
    void mouseMoveEvent(QMouseEvent * event) override;    // 鼠标移动
    void mouseReleaseEvent(QMouseEvent * event) override; // 鼠标释放
    void resizeEvent(QResizeEvent * event) override;
    void timerEvent(QTimerEvent * event) override;
    bool eventFilter(QObject * obj, QEvent * event) override;
    void dragEnterEvent(QDragEnterEvent * event) override;
    void dropEvent(QDropEvent * event) override;

private slots:
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

    void onVolumeButtonEnter();
    void onVolumeChanged(int vol);
    void onLstDbclickedRecord(QListWidgetItem * item);
    void onActionsVideoTriggered(QAction * action);
    void onActionsAudioTriggered(QAction * action);

private:
    // 居中
    void moveCenter();
    // 播放
    void play(const std::string & url);
    // 
    void cleanup();
    // 加载配置
    bool loadConfig();
    // 卸载配置
    void unloadConfig();
    // 全屏切换
    void toggleFullScreen();
    // 获取显示区域大小
    void getDisplaySize(int & width, int & height);

private:
    Ui::XPlayerClass ui;

    CVolumeWidget * m_pVolumeWidget = nullptr;
    QTimer * m_tmVolume = nullptr;

    QPoint m_ptStart;
    bool m_blPressed = false;
    int m_iTid = -1;

    QMenu * m_pclsChoices = nullptr;
    QMenu * m_pmnuVideo = nullptr;
    QMenu * m_pmnuAudio = nullptr;
    QActionGroup * m_grpAudioActions = nullptr;
    QActionGroup * m_grpVideoActions = nullptr;

    CRecordWidget * m_pVodWidget = nullptr;
    CRecordWidget * m_pLiveWidget = nullptr;

    // 是否全屏
    bool m_blFullScreen = false;
    // 
    QWidget * m_wndScreenParent = nullptr;

    // 倍速
    uint32_t m_uiSpeed = 0;
};
