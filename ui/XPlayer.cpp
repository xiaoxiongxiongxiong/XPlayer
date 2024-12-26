#include "XPlayer.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QResizeEvent>
#include <QMenuBar>
#include <QPainter>
#include <windows.h>

#include "utils/xplayer_utils.h"
#include "renderer/xplayer_audio_render.h"
#include "renderer/xplayer_video_render_sdl.h"
#include "xplayer_source.h"

XPlayer::XPlayer(QWidget * parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    this->setWindowFlags(Qt::FramelessWindowHint); // Qt::SplashScreen | Qt::FramelessWindowHint);

    ui.m_actVod->setIcon(QIcon(":/XPlayer/res/vod.ico"));
    ui.m_actLive->setIcon(QIcon(":/XPlayer/res/live.ico"));

    ui.m_actVod->setShortcut(QKeySequence::Open);
    ui.m_actLive->setShortcut(QKeySequence::Underline);

    m_pclsChoices = new QMenu(ui.m_wndTitle);
    m_pclsChoices->addAction(ui.m_actVod);
    m_pclsChoices->addAction(ui.m_actLive);

    m_pclsChoices->addSeparator();

    auto * video_mnu = m_pclsChoices->addMenu(QStringLiteral("视频选项"));
    video_mnu->addAction(ui.m_actDisableVideo);

    auto * audio_mnu = m_pclsChoices->addMenu(QStringLiteral("音频选项"));
    audio_mnu->addAction(ui.m_actDisableAudio);

    m_pclsChoices->setStyleSheet(R"(
        QMenu {
            background-color: #1f1f1f;
            color: white;
        }
        QMenu::item {
            padding: 2px 20px 2px 20px;
        }
        QMenu::item:selected {
            background-color: #323232;
            color: white;
        }
    )");
    ui.m_btnChoice->setMenu(m_pclsChoices);

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
    const QString strFilter = tr("mp4(*.mp4);;mpegts(*.ts);;All Files(*.*)");
    QString strFileName = QFileDialog::getOpenFileName(this, QStringLiteral("文件对话框"), "F:\\media", strFilter);
    if (strFileName.isEmpty())
    {
        return;
    }

    std::string path;
    auto strFileArray = strFileName.toLocal8Bit();
    path.assign(strFileArray.constData(), strFileArray.length());
    play(path);
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

    auto title_size = ui.m_wndTitle->size();
    title_size.setWidth(width);
    ui.m_wndTitle->resize(title_size);

    const auto center = width / 2 - 25;

    const auto bh = 0;// ui.m_barMenu->size().height();

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

void XPlayer::play(const std::string & url)
{
    if (!CXPlayerSource::getInstance().open(url))
    {
        QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("打开失败！"));
        return;
    }

    int64_t duration_ms = CXPlayerSource::getInstance().duration();
    if (duration_ms > 0)
    {
        ui.m_sldProgress->setEnabled(true);
        ui.m_sldProgress->setMaximum(static_cast<int>(duration_ms));
    }
    else
        ui.m_sldProgress->setEnabled(false);

    std::vector<int> ais;
    std::vector<int> vis;
    CXPlayerSource::getInstance().getStreamsInfo(ais, vis);

    for (auto iter = ais.cbegin(); iter != ais.cend(); ++iter)
    {
        const auto index = static_cast<int>(std::distance(ais.cbegin(), iter));
        auto * act = new QAction(this);
        act->setText(QStringLiteral("音轨%1").arg(index));
        act->setData(QVariant::fromValue(*iter));
        act->setObjectName(QStringLiteral("m_actAudio%d").arg(index));
        //ui.m_mnuAudio->addAction(act);
    }

    for (auto iter = vis.cbegin(); iter != vis.cend(); iter++)
    {
        const auto index = static_cast<int>(std::distance(vis.cbegin(), iter));
        auto * act = new QAction(this);
        act->setText(QStringLiteral("视轨%1").arg(index));
        act->setData(QVariant::fromValue(*iter));
        act->setObjectName(QStringLiteral("m_actVideo%d").arg(index));
        act->setChecked(true);
        //ui.m_mnuVideo->addAction(act);
    }
}
