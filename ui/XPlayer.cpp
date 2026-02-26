#include "XPlayer.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QResizeEvent>
#include <QMenuBar>
#include <QPainter>
#include <QFileInfo>
#include <windows.h>

#include "CVolumeWidget.h"
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

    m_pclsChoices = new QMenu(ui.m_btnChoice);
    m_pclsChoices->addAction(ui.m_actVod);
    m_pclsChoices->addAction(ui.m_actLive);

    m_pclsChoices->addSeparator();

    m_pmnuVideo = m_pclsChoices->addMenu(QStringLiteral("视频选项"));
    m_pmnuVideo->addAction(ui.m_actDisableVideo);

    m_pmnuAudio = m_pclsChoices->addMenu(QStringLiteral("音频选项"));
    m_pmnuAudio->addAction(ui.m_actDisableAudio);

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
    connect(ui.m_btnRecord, SIGNAL(clicked()), this, SLOT(onBtnClickedRecord()));

    ui.m_btnNext->setToolTip(QStringLiteral("下一个"));
    ui.m_btnLast->setToolTip(QStringLiteral("上一个"));
    ui.m_btnBackward->setToolTip(QStringLiteral("快退"));
    ui.m_btnForward->setToolTip(QStringLiteral("快进"));
    ui.m_btnStop->setToolTip(QStringLiteral("停止"));
    ui.m_btnCtrl->setToolTip(QStringLiteral("播放"));

    ui.m_lstRecord->hide();
    ui.m_lstRecord->addItem(QStringLiteral("小红帽与大灰狼"));

    ui.m_sldProgress->installEventFilter(this);

    // 创建音量滑块（初始隐藏）
    m_widgetVolume = new CVolumeWidget(this);
    m_widgetVolume->hide();

    ui.m_btnVolume->installEventFilter(this);

    // 悬停显示逻辑：加一点延迟防止误触
    m_tmVolume = new QTimer(this);
    m_tmVolume->setSingleShot(true);
    m_tmVolume->setInterval(200);
    connect(m_tmVolume, &QTimer::timeout, this, &XPlayer::onVolumeButtonEnter);
    connect(m_widgetVolume, &CVolumeWidget::volumeChanged, this, &XPlayer::onVolumeChanged);

    connect(ui.m_sldProgress, &QSlider::valueChanged, this, &XPlayer::onProgressChanged);
}

XPlayer::~XPlayer()
{
    if (m_pclsChoices)
    {
        delete m_pclsChoices;
        m_pclsChoices = nullptr;
    }

    if (nullptr != m_widgetVolume)
    {
        delete m_widgetVolume;
        m_widgetVolume = nullptr;
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
    CXPlayerSource::getInstance().close();
    QApplication * app;
    app->quit();
}

void XPlayer::onBtnClickedVolume()
{
    auto val = m_widgetVolume->getVolume();
    if (val > 0)
    {
        ui.m_btnVolume->setIcon(QIcon(":/XPlayer/res/silence.ico"));
        m_widgetVolume->setVolume(0);
    }
    else
    {
        m_widgetVolume->setVolume(50);
        ui.m_btnVolume->setIcon(QIcon(":/XPlayer/res/voice.ico"));
    }
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
    CXPlayerSource::getInstance().close();
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

void XPlayer::onBtnClickedRecord()
{
    static bool flag = false;
    if (flag)
        ui.m_lstRecord->hide();
    else
        ui.m_lstRecord->show();
    flag = !flag;
}

void XPlayer::paintEvent(QPaintEvent * event)
{
    QPainter painter(this);
    painter.fillRect(0, 0, this->height(), this->width(), QColor(34, 39, 56));
}

void XPlayer::onVolumeButtonEnter()
{
    if (!m_widgetVolume->isVisible())
    {
        QRect rect = ui.m_btnVolume->rect();
        QPoint tlr = ui.m_btnVolume->mapToGlobal(rect.topLeft());
        int height = m_widgetVolume->height();
        QPoint pos(tlr.x(), tlr.y() - height);
        m_widgetVolume->showVolume(pos);
    }
}

void XPlayer::onVolumeButtonLeave()
{
    // 由 VolumeWidget 自己处理隐藏（通过 leaveEvent + timer）
}

void XPlayer::onVolumeChanged(int vol)
{
    if (0 == vol)
        ui.m_btnVolume->setIcon(QIcon(":/XPlayer/res/silence.ico"));
    else
        ui.m_btnVolume->setIcon(QIcon(":/XPlayer/res/voice.ico"));
}

void XPlayer::onProgressChanged(int val)
{
    auto pos = val;
}

bool XPlayer::eventFilter(QObject * obj, QEvent * event)
{
    if (obj == ui.m_sldProgress && (QEvent::MouseButtonPress == event->type() || QEvent::MouseButtonRelease == event->type()))
    {
        auto * ev = static_cast<QMouseEvent *>(event);
        if (Qt::LeftButton == ev->button())
        {
            // 获取点击位置
            QPoint pos = ev->pos();

            // 水平滑块（备用）
            int width = ui.m_sldProgress->width();
            double ratio = static_cast<double>(pos.x()) / width;
            ratio = qBound(0.0, ratio, 1.0);

            int range = ui.m_sldProgress->maximum() - ui.m_sldProgress->minimum();
            int value = ui.m_sldProgress->minimum() + qRound(ratio * range);
            ui.m_sldProgress->setValue(value);
            return true;
        }
    }

    if (obj == ui.m_btnVolume)
    {
        if (QEvent::Enter == event->type())
        {
            m_tmVolume->start();
            return true;
        }
        else if (QEvent::Leave == event->type())
        {
            m_tmVolume->stop();
            m_widgetVolume->hideVolume();
            return true;
        }
        else if (QEvent::Wheel == event->type())
        {
            QWheelEvent * wheel = static_cast<QWheelEvent *>(event);
            //if (!m_isMuted) {
            int delta = wheel->angleDelta().y();
            int step = (delta > 0) ? 1 : -1;
            int current = m_widgetVolume->getVolume();
            int vol = qBound(0, current + step, 100);

            m_widgetVolume->setVolume(vol);
            onVolumeChanged(vol);
            //}
            return true;
        }
    }
    return QMainWindow::eventFilter(obj, event);
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
        m_pmnuAudio->addAction(act);
    }

    for (auto iter = vis.cbegin(); iter != vis.cend(); iter++)
    {
        const auto index = static_cast<int>(std::distance(vis.cbegin(), iter));
        auto * act = new QAction(this);
        act->setText(QStringLiteral("视轨%1").arg(index));
        act->setData(QVariant::fromValue(*iter));
        act->setObjectName(QStringLiteral("m_actVideo%d").arg(index));
        act->setChecked(true);
        m_pmnuVideo->addAction(act);
    }

    const auto width = ui.m_wndScreen->width();
    const auto height = ui.m_wndScreen->height();
    CXPlayerSource::getInstance().play(reinterpret_cast<HWND>(ui.m_wndScreen->winId()), width, height);
}
