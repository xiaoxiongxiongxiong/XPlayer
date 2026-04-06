#include "XPlayer.h"

#include <QScreen>
#include <QFileDialog>
#include <QMessageBox>
#include <QResizeEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QMenuBar>
#include <QPainter>
#include <QFileInfo>
#include <windows.h>

#include "RecordWidget.h"
#include "CVolumeWidget.h"
#include "utils/xplayer_utils.h"
#include "config/xplayer_config.h"
#include "renderer/xplayer_audio_render_sdl.h"
#include "renderer/xplayer_video_render_sdl.h"
#include "xplayer_source.h"

XPlayer::XPlayer(QWidget * parent)
    : FramelessMainWindow(parent)
{
    ui.setupUi(this);

    //this->setWindowFlags(Qt::FramelessWindowHint);
    this->setAcceptDrops(true);

    ui.m_actDisableAudio->setData(QVariant::fromValue(-1));
    ui.m_actDisableVideo->setData(QVariant::fromValue(-1));

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

    m_grpAudioActions = new QActionGroup(this);
    m_grpAudioActions->setExclusive(true);
    m_grpAudioActions->addAction(ui.m_actDisableAudio);
    connect(m_grpAudioActions, &QActionGroup::triggered, this, &XPlayer::onActionsAudioTriggered);

    m_grpVideoActions = new QActionGroup(this);
    m_grpVideoActions->setExclusive(true);
    m_grpVideoActions->addAction(ui.m_actDisableVideo);
    connect(m_grpVideoActions, &QActionGroup::triggered, this, &XPlayer::onActionsVideoTriggered);

    ui.m_actDisableAudio->setCheckable(true);
    ui.m_actDisableVideo->setCheckable(true);

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
    ui.m_btnBackward->setToolTip(QStringLiteral("慢进"));
    ui.m_btnForward->setToolTip(QStringLiteral("快进"));
    ui.m_btnStop->setToolTip(QStringLiteral("停止"));
    ui.m_btnCtrl->setToolTip(QStringLiteral("播放"));
    ui.m_btnCtrl->setShortcut(QKeySequence(Qt::Key_Space));

    ui.m_tabRecord->hide();

    m_pVodWidget = new CRecordWidget(this);
    ui.m_tabRecord->addTab(m_pVodWidget, QStringLiteral("文件"));
    connect(m_pVodWidget, &CRecordWidget::itemDbclicked, this, &XPlayer::onLstDbclickedRecord);

    m_pLiveWidget = new CRecordWidget(this);
    ui.m_tabRecord->addTab(m_pLiveWidget, QStringLiteral("链接"));
    connect(m_pLiveWidget, &CRecordWidget::itemDbclicked, this, &XPlayer::onLstDbclickedRecord);

    ui.m_sldProgress->installEventFilter(this);
    ui.m_wndScreen->installEventFilter(this);
    ui.m_wndScreen->setFocusPolicy(Qt::StrongFocus);

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

    loadConfig();
    m_uiSpeed = XPLAYER_SPEED_NORMAL;

    const QString font_path = QStringLiteral("test.ttc");
    CXPlayerSource::getInstance().setFontPath(font_path.toUtf8().toStdString());
    CXPlayerSource::getInstance().setFontSize(28);
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

    unloadConfig();
}

void XPlayer::keyPressEvent(QKeyEvent * event)
{
    QWidget::keyPressEvent(event);
}

void XPlayer::keyReleaseEvent(QKeyEvent * event)
{
    if (Qt::Key_Tab == event->key())
    {
        static bool flag = true;
        CXPlayerSource::uniqueInstance().showDetail(flag);
        flag = !flag;
    }

    QWidget::keyReleaseEvent(event);
}

void XPlayer::mousePressEvent(QMouseEvent * event)
{
    if (event->button() != Qt::LeftButton)
    {
        QMainWindow::mousePressEvent(event);
        return;
    }

    m_blPressed = true;
    m_ptStart = event->globalPos() - frameGeometry().topLeft();
}

void XPlayer::mouseMoveEvent(QMouseEvent * event)
{
    if (m_blPressed)
    {
        move(event->globalPos() - m_ptStart);
        return;
    }

    QMainWindow::mouseMoveEvent(event);
}

void XPlayer::mouseReleaseEvent(QMouseEvent * event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_blPressed = false;
    }
    QMainWindow::mouseReleaseEvent(event);
}

void XPlayer::resizeEvent(QResizeEvent * event)
{
    if (XPLAYER_STATE_NONE != CXPlayerSource::uniqueInstance().state())
    {
        int width = 0, height = 0;
        getDisplaySize(width, height);
        CXPlayerSource::uniqueInstance().resize(width, height);
    }
}

void XPlayer::timerEvent(QTimerEvent * event)
{
    if (m_iTid == event->timerId() && XPLAYER_STATE_NONE != CXPlayerSource::uniqueInstance().state())
    {
        if (XPLAYER_STATE_OVER == CXPlayerSource::uniqueInstance().state())
        {
            cleanup();
            return;
        }

        auto pos = static_cast<int>(CXPlayerSource::uniqueInstance().progress());
        if (pos > 0)
            ui.m_sldProgress->setValue(pos);
    }
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
            CXPlayerSource::uniqueInstance().seek(value);
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
            int vol = qBound(0, current + step, 128);

            m_widgetVolume->setVolume(vol);
            onVolumeChanged(vol);
            //}
            return true;
        }
    }
    
    if (obj == ui.m_wndScreen && QEvent::MouseButtonDblClick == event->type())
    {
        toggleFullScreen();

        return true;
    }

    return QMainWindow::eventFilter(obj, event);
}

void XPlayer::dragEnterEvent(QDragEnterEvent * event)
{
    if (!event->mimeData()->hasUrls())
    {
        event->ignore();
        return;
    }

    event->acceptProposedAction();
}

void XPlayer::dropEvent(QDropEvent * event)
{
    const QMimeData * mime_data = event->mimeData();
    if (!mime_data->hasUrls())
        return;

    QList<QUrl> urls = mime_data->urls();
    if (1 != urls.size())
        return;

    cleanup();

    QString strFileName = urls[0].toLocalFile();
    QFileInfo fileInfo(strFileName);
    ui.m_labName->setText(QStringLiteral("%1").arg(fileInfo.fileName()));

    m_pVodWidget->addRecord(fileInfo.fileName(), strFileName);

    play(strFileName.toStdString());
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
    cleanup();
    QApplication * app;
    app->quit();
}

void XPlayer::onBtnClickedVolume()
{
    auto val = m_widgetVolume->getVolume();
    if (val > 0)
    {
        m_widgetVolume->setVolume(0);
        CXPlayerSource::uniqueInstance().setVolume(0);
        ui.m_btnVolume->setIcon(QIcon(":/XPlayer/res/silence.ico"));
    }
    else
    {
        m_widgetVolume->setVolume(50);
        CXPlayerSource::uniqueInstance().setVolume(64);
        ui.m_btnVolume->setIcon(QIcon(":/XPlayer/res/voice.ico"));
    }
}

void XPlayer::onBtnClickedVod()
{
    const QString strFilter = tr("mp4(*.mp4);;mpegts(*.ts);;All Files(*.*)");
    QString strFileName = QFileDialog::getOpenFileName(this, QStringLiteral("文件对话框"), "F:\\media", strFilter);
    if (strFileName.isEmpty())
        return;

    cleanup();

    QFileInfo fileInfo(strFileName);
    ui.m_labName->setText(QStringLiteral("%1").arg(fileInfo.fileName()));

    m_pVodWidget->addRecord(fileInfo.fileName(), strFileName);

    play(strFileName.toStdString());
}

void XPlayer::onBtnClickedLive()
{
}

void XPlayer::onBtnClickedCtrl()
{
    const auto & state = CXPlayerSource::uniqueInstance().state();
    if (XPLAYER_STATE_NONE == state)
        return;

    if (XPLAYER_STATE_PLAYING == state)
    {
        ui.m_btnCtrl->setIcon(QIcon(":/XPlayer/res/pause.ico"));
        ui.m_btnCtrl->setToolTip(QStringLiteral("播放"));
        CXPlayerSource::uniqueInstance().pause(true);
    }
    else if (XPLAYER_STATE_PAUSE == state)
    {
        ui.m_btnCtrl->setIcon(QIcon(":/XPlayer/res/play.ico"));
        ui.m_btnCtrl->setToolTip(QStringLiteral("暂停"));
        CXPlayerSource::uniqueInstance().pause(false);
    }
}

void XPlayer::onBtnClickedStop()
{
    cleanup();
}

void XPlayer::onBtnClickedBackward()
{
    if (m_uiSpeed > XPLAYER_SPEED_ONE_QUATER)
    {
        m_uiSpeed--;
        CXPlayerSource::uniqueInstance().setSpeed(static_cast<XPLAYER_SPEED_MODE>(m_uiSpeed));
    }
}

void XPlayer::onBtnClickedForward()
{
    if (m_uiSpeed < XPLAYER_SPEED_QUADRUPLE)
    {
        m_uiSpeed++;
        CXPlayerSource::uniqueInstance().setSpeed(static_cast<XPLAYER_SPEED_MODE>(m_uiSpeed));
    }
}

void XPlayer::onBtnClickedLast()
{
    if (nullptr != m_pVodWidget)
    {
        m_pVodWidget->lastRecord();
    }
}

void XPlayer::onBtnClickedNext()
{
    if (nullptr != m_pVodWidget)
    {
        m_pVodWidget->nextRecord();
    }
}

void XPlayer::onBtnClickedRecord()
{
    auto flag = CXPlayerConfig::uniqueInstance().getRecordVisible();
    ui.m_tabRecord->setVisible(!flag);
    CXPlayerConfig::uniqueInstance().setRecordVisible(!flag);

    ui.horizontalLayout->activate();

    if (XPLAYER_STATE_NONE != CXPlayerSource::uniqueInstance().state())
    {
        int width = 0, height = 0;
        getDisplaySize(width, height);
        CXPlayerSource::uniqueInstance().resize(width, height);
    }
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

void XPlayer::onVolumeChanged(int vol)
{
    if (0 == vol)
        ui.m_btnVolume->setIcon(QIcon(":/XPlayer/res/silence.ico"));
    else
        ui.m_btnVolume->setIcon(QIcon(":/XPlayer/res/voice.ico"));
    CXPlayerSource::uniqueInstance().setVolume(vol);
    CXPlayerConfig::uniqueInstance().setVolume(vol);
}

void XPlayer::onLstDbclickedRecord(QListWidgetItem * item)
{
    cleanup();

    auto path = item->data(Qt::UserRole + 1).toString();
    ui.m_labName->setText(item->text());
    play(path.toStdString());
}

void XPlayer::onActionsVideoTriggered(QAction * action)
{
    auto index = action->data().toInt();
    CXPlayerSource::uniqueInstance().selectStream(index, true);
}

void XPlayer::onActionsAudioTriggered(QAction * action)
{
    auto index = action->data().toInt();
    CXPlayerSource::uniqueInstance().selectStream(index, false);
}

void XPlayer::play(const std::string & url)
{
    if (!CXPlayerSource::uniqueInstance().open(url))
    {
        auto * err = CXPlayerSource::uniqueInstance().err();
        QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("%1！").arg(err));
        return;
    }

    int64_t duration_ms = CXPlayerSource::uniqueInstance().duration();
    if (duration_ms > 0)
    {
        ui.m_sldProgress->setEnabled(true);
        ui.m_sldProgress->setMaximum(static_cast<int>(duration_ms));
    }
    else
        ui.m_sldProgress->setEnabled(false);

    std::vector<int> ais;
    std::vector<int> vis;
    CXPlayerSource::uniqueInstance().getStreamsInfo(ais, vis);

    for (auto iter = ais.cbegin(); iter != ais.cend(); ++iter)
    {
        const auto index = static_cast<int>(std::distance(ais.cbegin(), iter));
        auto * act = new QAction(this);
        act->setCheckable(true);
        act->setChecked(0 == index);
        act->setText(QStringLiteral("音轨%1").arg(index));
        act->setData(QVariant::fromValue(*iter));
        act->setObjectName(QStringLiteral("m_actAudio%d").arg(index));
        m_pmnuAudio->addAction(act);
        m_grpAudioActions->addAction(act);
    }

    for (auto iter = vis.cbegin(); iter != vis.cend(); iter++)
    {
        const auto index = static_cast<int>(std::distance(vis.cbegin(), iter));
        auto * act = new QAction(this);
        act->setCheckable(true);
        act->setChecked(0 == index);
        act->setText(QStringLiteral("视轨%1").arg(index));
        act->setData(QVariant::fromValue(*iter));
        act->setObjectName(QStringLiteral("m_actVideo%d").arg(index));
        m_pmnuVideo->addAction(act);
        m_grpVideoActions->addAction(act);
    }

    // 为了解决SDL_DestroyWindow后画面显示问题
    ui.m_wndScreen->hide();
    ui.m_wndScreen->show();
    int width = 0, height = 0;
    getDisplaySize(width, height);
    if (!CXPlayerSource::uniqueInstance().play(reinterpret_cast<HWND>(ui.m_wndScreen->winId()), width, height))
    {
        auto * err = CXPlayerSource::uniqueInstance().err();
        QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("%1！").arg(err));
        return;
    }
    ui.m_btnCtrl->setIcon(QIcon(":/XPlayer/res/play.ico"));
    ui.m_btnCtrl->setToolTip(QStringLiteral("暂停"));
    m_iTid = startTimer(std::chrono::milliseconds(10));
}

void XPlayer::cleanup()
{
    if (XPLAYER_STATE_NONE != CXPlayerSource::uniqueInstance().state())
    {
        CXPlayerSource::uniqueInstance().close();
    }

    if (m_iTid > -1)
    {
        killTimer(m_iTid);
        m_iTid = -1;
    }
    ui.m_labName->clear();
    ui.m_btnCtrl->setIcon(QIcon(":/XPlayer/res/pause.ico"));
    ui.m_sldProgress->setValue(0);

    auto actions = m_grpVideoActions->actions();
    for (auto & action : actions)
    {
        if (ui.m_actDisableVideo->objectName() == action->objectName())
            continue;
        m_grpVideoActions->removeAction(action);
    }

    actions = m_grpAudioActions->actions();
    for (auto & action : actions)
    {
        if (ui.m_actDisableAudio->objectName() == action->objectName())
            continue;
        m_grpAudioActions->removeAction(action);
    }

    actions = m_pmnuAudio->actions();
    for (auto & action : actions)
    {
        if (ui.m_actDisableAudio->objectName() == action->objectName())
            continue;

        m_pmnuAudio->removeAction(action);
        if (nullptr == action->parent())
            delete action;
    }

    actions = m_pmnuVideo->actions();
    for (auto & action : actions)
    {
        if (ui.m_actDisableVideo->objectName() == action->objectName())
            continue;

        m_pmnuVideo->removeAction(action);
        if (nullptr == action->parent())
            delete action;
    }
}

bool XPlayer::loadConfig()
{
    const auto path = QCoreApplication::applicationDirPath();
    const auto strConfigPath = path + "/config/config.json";
    const auto strVodPath = path + "/config/vod.json";
    const auto strLivePath = path + "/config/live.json";

    CXPlayerConfig::uniqueInstance().loadConfig(strConfigPath.toLocal8Bit().toStdString());
    m_pVodWidget->loadRecord(strVodPath);
    m_pLiveWidget->loadRecord(strLivePath);

    const auto strFontPath = path + QStringLiteral("/fonts/微软雅黑.ttc");
    CXPlayerConfig::uniqueInstance().setFontPath(strFontPath.toUtf8().toStdString());
    CXPlayerSource::uniqueInstance().setFontPath(strFontPath.toUtf8().toStdString());
    CXPlayerSource::uniqueInstance().setFontSize(CXPlayerConfig::uniqueInstance().getFontSize());

    m_pVolumeWidget->setVolume(CXPlayerConfig::uniqueInstance().getVolume());
    CXPlayerSource::uniqueInstance().setVolume(CXPlayerConfig::uniqueInstance().getVolume());
    ui.m_tabRecord->setVisible(CXPlayerConfig::uniqueInstance().getRecordVisible());

    return true;
}

void XPlayer::unloadConfig()
{
    CXPlayerConfig::uniqueInstance().unloadConfig();

    if (nullptr != m_pVodWidget)
    {
        m_pVodWidget->unloadRecord();
        delete m_pVodWidget;
        m_pVodWidget = nullptr;
    }

    if (nullptr != m_pLiveWidget)
    {
        m_pLiveWidget->unloadRecord();
        delete m_pLiveWidget;
        m_pLiveWidget = nullptr;
    }
}

void XPlayer::toggleFullScreen()
{
    if (!m_blFullScreen)
    {
        m_wndScreenParent = ui.m_wndScreen->parentWidget();
        ui.m_wndScreen->setParent(nullptr);
        this->hide();
        ui.m_wndScreen->showFullScreen();
        m_blFullScreen = true;
    }
    else
    {
        ui.m_wndScreen->showNormal();
        ui.m_wndScreen->setParent(m_wndScreenParent);
        ui.verticalLayout_2->insertWidget(0, ui.m_wndScreen);
        this->show();
        this->activateWindow();
        m_blFullScreen = false;
    }

    if (XPLAYER_STATE_NONE != CXPlayerSource::uniqueInstance().state())
    {
        int width = 0, height = 0;
        getDisplaySize(width, height);
        CXPlayerSource::uniqueInstance().resize(width, height);
    }
}

void XPlayer::getDisplaySize(int & width, int & height)
{
    auto * screen = QGuiApplication::primaryScreen();
    auto size = ui.m_wndScreen->size();
    auto ratio = screen->devicePixelRatio();
    width = size.width() * ratio;
    height = size.height() * ratio;
}
