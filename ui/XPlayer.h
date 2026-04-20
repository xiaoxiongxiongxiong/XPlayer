#pragma once

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QListWidget>
#include "ui_XPlayer.h"

class QMenuBar;
class CVolumeWidget;
class CRecordWidget;

class XPlayer : public QMainWindow
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
    void onVideoRendererTriggered(QAction * action);
    void onVideoTracksTriggered(QAction * action);
    void onAudioTracksTriggered(QAction * action);

    void onFrameReady();

private:
    // 居中
    void moveCenter();

    // 初始化菜单栏
    void initMenuBar();

    // 更新鼠标形状
    void updateCursorShape(const QPoint & pt);

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

    // 边界
    uint32_t m_uiEdge = 0u;
    QPoint m_ptStart;
    QRect m_recStart;
    bool m_blPressed = false;

    QMenu * m_pmnuVideoRenderers = nullptr;
    QMenu * m_pmnuVideoTracks = nullptr;
    QMenu * m_pmnuAudioTracks = nullptr;

    QActionGroup * m_grpVideoRenderers = nullptr;
    QActionGroup * m_grpVideoTracks = nullptr;
    QActionGroup * m_grpAudioTracks = nullptr;

    CRecordWidget * m_pVodWidget = nullptr;
    CRecordWidget * m_pLiveWidget = nullptr;

    // 是否全屏
    bool m_blFullScreen = false;
    // 
    QWidget * m_wndScreenParent = nullptr;

    int m_iTid = -1;
    // 倍速
    uint32_t m_uiSpeed = 0;
};
