#include "XPlayer.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QResizeEvent>
#include <QMenuBar>
#include <QPainter>
#include <QFileInfo>
#include <QPropertyAnimation>
#include <windows.h>

#include "utils/xplayer_utils.h"
#include "renderer/xplayer_audio_render_sdl.h"
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

    // 创建位置动画
    ani = new QPropertyAnimation(ui.m_labName, "geometry");
    ani->setDuration(6000); // 动画持续时间6秒
    ani->setLoopCount(-1);
    ani->start();

    connect(ui.m_actVod, &QAction::triggered, this, &XPlayer::onBtnClickedVod);
    connect(ui.m_actLive, &QAction::triggered, this, &XPlayer::onBtnClickedLive);

    connect(ui.m_btnMinimize, SIGNAL(clicked()), this, SLOT(onBtnClickedMinimize()));
    connect(ui.m_btnMaximize, SIGNAL(clicked()), this, SLOT(onBtnClickedMaximize()));
    connect(ui.m_btnClose, SIGNAL(clicked()), this, SLOT(onBtnClickedClose()));

    connect(ui.m_btnVolume, SIGNAL(clicked()), this, SLOT(onBtnClickedVolume()));
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

void XPlayer::onBtnClickedMinimize()
{
    if (Qt::WindowMinimized == this->windowState())
        this->showNormal();
    else
        this->showMinimized();
}

void XPlayer::onBtnClickedMaximize()
{
    if (Qt::WindowMaximized == this->windowState())
        this->showNormal();
    else
        this->showMaximized();
}

void XPlayer::onBtnClickedClose()
{
    QApplication * app;
    app->quit();
}

void XPlayer::onBtnClickedVolume()
{
    auto val = ui.m_sldVolume->value();
    if (val > 0)
    {
        ui.m_btnVolume->setIcon(QIcon(":/XPlayer/res/silence.ico"));
        ui.m_sldVolume->setProperty("user_id", val);
        ui.m_sldVolume->setValue(0);
    }
    else
    {
        ui.m_sldVolume->setValue(ui.m_sldVolume->property("user_id").toInt());
        ui.m_btnVolume->setIcon(QIcon(":/XPlayer/res/voice.ico"));
    }

    //static bool flag = false;
    //if (flag)
    //    ui.m_btnVolume->setIcon(QIcon(":/XPlayer/res/voice.ico"));
    //else
    //{
    //    ui.m_btnVolume->setIcon(QIcon(":/XPlayer/res/silence.ico"));
    //    ui.m_sldVolume->setValue(0);
    //}
    //flag = !flag;
}

void XPlayer::onBtnClickedVod()
{
    const QString strFilter = tr("mp4(*.mp4);;mpegts(*.ts);;All Files(*.*)");
    QString strFileName = QFileDialog::getOpenFileName(this, QStringLiteral("文件对话框"), "F:\\media", strFilter);
    if (strFileName.isEmpty())
        return;

    QFileInfo fileInfo(strFileName);
    ui.m_labName->setText(QStringLiteral("%1").arg(fileInfo.fileName()));

    play(strFileName.toStdString());
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
    painter.fillRect(0, 0, this->height(), this->width(), QColor(34, 39, 56));
}

void XPlayer::resizeEvent(QResizeEvent * event)
{
    const auto height = event->size().height();
    const auto width = event->size().width();

    // 标题栏处理
    auto title_size = ui.m_wndTitle->size();
    title_size.setWidth(width);
    ui.m_wndTitle->resize(title_size);

    auto bw = ui.m_btnMinimize->size().width();
    ui.m_btnMinimize->move(width - bw * 3, 0);
    ui.m_btnMaximize->move(width - bw * 2, 0);
    ui.m_btnClose->move(width - bw, 0);

    auto tmp = ui.m_labName->size();
    ui.m_labName->move((width - bw * 3 - 60) / 2, 2);
    //auto lab_pos = ui.m_labName->geometry();
    //ani->setStartValue(ui.m_labName->geometry()); // 初始位置和大小
    //lab_pos.setLeft(lab_pos.left() - tmp.width() / 2);
    //lab_pos.setWidth(tmp.width());
    //ani->setEndValue(lab_pos); // 结束位置和大小

    // 操作栏处理
    auto ctrl_size = ui.m_wndCtrl->size();
    ctrl_size.setWidth(width);
    ui.m_wndCtrl->resize(ctrl_size);
    ui.m_wndCtrl->move(0, height - ctrl_size.height());

    const auto btn_size = ui.m_btnCtrl->size();
    bw = btn_size.width();
    auto bh = (ctrl_size.height() - btn_size.height()) / 2;
    const auto center = width / 2 - bw / 2;
    ui.m_btnLast->move(center - bw * 2, bh);
    ui.m_btnBackward->move(center - bw, bh);
    ui.m_btnCtrl->move(center, bh);
    ui.m_btnForward->move(center + bw, bh);
    ui.m_btnNext->move(center + bw * 2, bh);
    ui.m_btnStop->move(5, bh);

    // 进度条 + 音量
    auto prg_size = ui.m_sldProgress->size();
    auto bvs = ui.m_btnVolume->size();
    auto slds = ui.m_sldVolume->size();
    auto val = bvs.width() + slds.width() + 10;
    auto sh = height - ctrl_size.height() - prg_size.height();
    prg_size.setWidth(width - val);
    ui.m_sldProgress->resize(prg_size);
    ui.m_sldProgress->move(0, sh);
    ui.m_btnVolume->move(width - val + 5, sh);
    ui.m_sldVolume->move(width - slds.width() - 5, sh);

    // 屏幕
    auto th = title_size.height() + ctrl_size.height() + prg_size.height();
    ui.m_wndScreen->move(0, title_size.height());
    ui.m_wndScreen->resize(event->size().width(), height - th);
}

void XPlayer::play(const std::string & url)
{
    if (!CXPlayerSource::getInstance().open(url))
    {
        auto * err = CXPlayerSource::getInstance().err();
        QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("%1！").arg(err));
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
