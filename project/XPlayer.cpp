#include "XPlayer.h"

#include <QResizeEvent>
#include <QMenuBar>
#include <windows.h>

#include "renderer/CVideoRender.h"
#include "renderer/CAudioRender.h"

XPlayer::XPlayer(QWidget * parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    this->setWindowFlags(Qt::SplashScreen | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);

    m_pclsMenuBar = this->menuBar();
    m_pclsMenuBar->setAutoFillBackground(true);
    this->setMenuBar(m_pclsMenuBar);

    QMenu * media_mnu = new QMenu(QStringLiteral("媒体"));
    QAction * file_action = new QAction(QStringLiteral("文件"));
    file_action->setIcon(QIcon(":/XPlayer/res/vod.ico"));
    //connect(file_action, &QAction::triggered, this, &ctv_media_player::onBtnClickedVod);
    media_mnu->addAction(file_action);

    QAction * link_action = new QAction(QStringLiteral("链接"));
    link_action->setIcon(QIcon(":/XPlayer/res/live.ico"));
    //connect(link_action, &QAction::triggered, this, &ctv_media_player::onBtnClickedLive);
    media_mnu->addAction(link_action);
    m_pclsMenuBar->addMenu(media_mnu);

    QMenu * video_mnu = new QMenu(QStringLiteral("视频"));
    QAction * video_action = new QAction(QStringLiteral("禁用"));
    video_mnu->addAction(video_action);
    m_pclsMenuBar->addMenu(video_mnu);

    QMenu * audio_mnu = new QMenu(QStringLiteral("音频"));
    QAction * audio_action = new QAction(QStringLiteral("禁用"));
    audio_mnu->addAction(audio_action);
    m_pclsMenuBar->addMenu(audio_mnu);

    QMenu * info_mnu = new QMenu(QStringLiteral("信息"));
    QAction * codec_action = new QAction(QStringLiteral("编解码器信息"));
    info_mnu->addAction(codec_action);

    QAction * error_action = new QAction(QStringLiteral("运行信息"));
    info_mnu->addAction(error_action);

    m_pclsMenuBar->addMenu(info_mnu);
}

XPlayer::~XPlayer()
{
    if (nullptr != m_pclsMenuBar)
    {
        delete m_pclsMenuBar;
        m_pclsMenuBar = nullptr;
    }
}

void XPlayer::mousePressEvent(QMouseEvent * event)
{
    m_blPressed = true; // 当前鼠标按下的即是QWidget而非界面上布局的其它控件
    m_lastPos = event->globalPos();
}

void XPlayer::mouseMoveEvent(QMouseEvent * event)
{
    if (m_blPressed)
    {
        int dx = event->globalX() - m_lastPos.x();
        int dy = event->globalY() - m_lastPos.y();
        m_lastPos = event->globalPos();
        move(x() + dx, y() + dy);
    }
}

void XPlayer::mouseReleaseEvent(QMouseEvent * event)
{
    int dx = event->globalX() - m_lastPos.x();
    int dy = event->globalY() - m_lastPos.y();
    move(x() + dx, y() + dy);
    m_blPressed = false; // 鼠标松开时，置为false
}


void XPlayer::paintEvent(QPaintEvent * event)
{
    QPainter painter(this);
    painter.fillRect(0, this->height() - 70, this->width(), 70, QColor(0, 0, 0));
}

void XPlayer::resizeEvent(QResizeEvent * event)
{
    const auto height = event->size().height();
    const auto width = event->size().width();

    const auto center = width / 2 - 25;

    const auto bh = m_pclsMenuBar->size().height();

    QPainter painter(this);
    painter.fillRect(0, this->height() - 70, this->width(), 70, QColor(0, 0, 0));

    ui.m_wndScreen->resize(event->size().width(), height - bh - 70);
    //_video_render->setWindowSize(event->size().width(), height - 60);

    ui.m_sldProgress->move(0, height - 70 - bh);
    ui.m_sldProgress->resize(width, 20);

    ui.m_btnStop->move(50, height - 50 - bh);

    ui.m_btnLast->move(center - 100, height - 50 - bh);
    ui.m_btnBackward->move(center - 50, height - 50 - bh);
    ui.m_btnCtrl->move(center, height - 50 - bh);
    ui.m_btnForward->move(center + 50, height - 50 - bh);
    ui.m_btnNext->move(center + 100, height - 50 - bh);
}
