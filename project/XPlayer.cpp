#include "XPlayer.h"

#include <QResizeEvent>
#include <QMenuBar>
#include <windows.h>

#include "renderer/xplayer_audio_render.h"
#include "renderer/xplayer_video_render.h"

XPlayer::XPlayer(QWidget * parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    this->setWindowFlags(Qt::SplashScreen | Qt::FramelessWindowHint);

    ui.m_actVod->setIcon(QIcon(":/XPlayer/res/vod.ico"));
    ui.m_actLive->setIcon(QIcon(":/XPlayer/res/live.ico"));

    connect(ui.m_actVod, &QAction::triggered, this, &XPlayer::onBtnClickedVod);
    connect(ui.m_actLive, &QAction::triggered, this, &XPlayer::onBtnClickedLive);

    connect(ui.m_btnCtrl, SIGNAL(clicked()), this, SLOT(onBtnClickedCtrl()));
    connect(ui.m_btnNext, SIGNAL(clicked()), this, SLOT(onBtnClickedNext()));
    connect(ui.m_btnLast, SIGNAL(clicked()), this, SLOT(onBtnClickedLast()));
    connect(ui.m_btnBackward, SIGNAL(clicked()), this, SLOT(onBtnClickedBackward()));
    connect(ui.m_btnForward, SIGNAL(clicked()), this, SLOT(onBtnClickedForward()));
    connect(ui.m_btnStop, SIGNAL(clicked()), this, SLOT(onBtnClickedStop()));
}

XPlayer::~XPlayer()
{
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

void XPlayer::onBtnClickedVod()
{
}

void XPlayer::onBtnClickedLive()
{
}

void XPlayer::onBtnClickedCtrl()
{
    static bool flag = false;
    if (flag)
        ui.m_btnCtrl->setIcon(QIcon(":/XPlayer/res/play.ico"));
    else
        ui.m_btnCtrl->setIcon(QIcon(":/XPlayer/res/pause.ico"));
    flag = !flag;
}

void XPlayer::onBtnClickedStop()
{
}

void XPlayer::onBtnClickedBackward()
{
}

void XPlayer::onBtnClickedForward()
{
}

void XPlayer::onBtnClickedLast()
{
}

void XPlayer::onBtnClickedNext()
{
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

    const auto bh = ui.m_barMenu->size().height();

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
