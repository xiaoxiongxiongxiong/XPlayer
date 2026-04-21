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

#include "MenuWidget.h"
#include "RecordWidget.h"
#include "VolumeWidget.h"
#include "LinkWidget.h"
#include "utils/xplayer_utils.h"
#include "config/xplayer_config.h"
#include "renderer/xplayer_audio_render_sdl.h"
#include "renderer/xplayer_video_render_sdl.h"
#include "xplayer_source.h"

XPlayer::XPlayer(QWidget * parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    setWindowFlags(Qt::FramelessWindowHint);
    setAcceptDrops(true);
    setMouseTracking(true);
    centralWidget()->setMouseTracking(true);
    QCoreApplication::instance()->installEventFilter(this);

    moveCenter();

    initMenuBar();

    connect(ui.m_btnVolume, SIGNAL(clicked()), this, SLOT(onBtnClickedVolume()));
    connect(ui.m_btnCtrl, SIGNAL(clicked()), this, SLOT(onBtnClickedCtrl()));
    connect(ui.m_btnNext, SIGNAL(clicked()), this, SLOT(onBtnClickedNext()));
    connect(ui.m_btnLast, SIGNAL(clicked()), this, SLOT(onBtnClickedLast()));
    connect(ui.m_btnBackward, SIGNAL(clicked()), this, SLOT(onBtnClickedBackward()));
    connect(ui.m_btnForward, SIGNAL(clicked()), this, SLOT(onBtnClickedForward()));
    connect(ui.m_btnStop, SIGNAL(clicked()), this, SLOT(onBtnClickedStop()));
    connect(ui.m_btnRecord, SIGNAL(clicked()), this, SLOT(onBtnClickedRecord()));

    ui.m_tabRecord->hide();

    m_pVodWidget = new CRecordWidget(this);
    ui.m_tabRecord->addTab(m_pVodWidget, QStringLiteral("文件"));
    connect(m_pVodWidget, &CRecordWidget::itemDbclicked, this, &XPlayer::onLstDbclickedRecord);

    m_pLiveWidget = new CRecordWidget(this);
    ui.m_tabRecord->addTab(m_pLiveWidget, QStringLiteral("链接"));
    connect(m_pLiveWidget, &CRecordWidget::itemDbclicked, this, &XPlayer::onLstDbclickedRecord);

    ui.m_sldProgress->installEventFilter(this);
    ui.m_wndScreen->installEventFilter(this);
    ui.m_wndScreen->setMouseTracking(true);
    ui.m_tabRecord->setMouseTracking(true);

    // 创建音量滑块（初始隐藏）
    m_pVolumeWidget = new CVolumeWidget(this);
    m_pVolumeWidget->hide();

    ui.m_btnVolume->installEventFilter(this);

    // 悬停显示逻辑：加一点延迟防止误触
    m_tmVolume = new QTimer(this);
    m_tmVolume->setSingleShot(true);
    m_tmVolume->setInterval(200);
    connect(m_tmVolume, &QTimer::timeout, this, &XPlayer::onVolumeButtonEnter);
    connect(m_pVolumeWidget, &CVolumeWidget::volumeChanged, this, &XPlayer::onVolumeChanged);

    connect(ui.m_wndScreen, &CXPlayerVideoRenderOpengl::frameReady, this, &XPlayer::onFrameReady);
    loadConfig();
    m_uiSpeed = XPLAYER_SPEED_NORMAL;
}

XPlayer::~XPlayer()
{
    if (nullptr != m_grpVideoRenderers)
    {
        delete m_grpVideoRenderers;
        m_grpVideoRenderers = nullptr;
    }

    if (nullptr != m_grpVideoTracks)
    {
        delete m_grpVideoTracks;
        m_grpVideoTracks = nullptr;
    }

    if (nullptr != m_grpAudioTracks)
    {
        delete m_grpAudioTracks;
        m_grpAudioTracks = nullptr;
    }

    if (nullptr != m_pVolumeWidget)
    {
        delete m_pVolumeWidget;
        m_pVolumeWidget = nullptr;
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
    m_ptStart = event->globalPos();
    m_recStart = geometry();
}

void XPlayer::mouseMoveEvent(QMouseEvent * event)
{
    if (m_blPressed)
    {
        const auto delta = event->globalPos() - m_ptStart;
        if (0 == m_uiEdge)
        {
            move(m_recStart.x() + delta.x(), m_recStart.y() + delta.y());
        }
        else
        {
            int x = m_recStart.x();
            int y = m_recStart.y();
            int w = m_recStart.width();
            int h = m_recStart.height();

            if (Qt::Edge::LeftEdge & m_uiEdge)
            {
                x += delta.x();
                w -= delta.x();
            }
            else if (Qt::Edge::RightEdge & m_uiEdge)
            {
                w += delta.x();
            }

            if (Qt::Edge::TopEdge & m_uiEdge)
            {
                y += delta.y();
                h -= delta.y();
            }
            else if (Qt::Edge::BottomEdge & m_uiEdge)
            {
                h += delta.y();
            }

            setGeometry(x, y, qMax(w, minimumWidth()), qMax(h, minimumHeight()));
        }
        return;
    }

    updateCursorShape(event->pos());

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

    if (QEvent::MouseMove == event->type() && !m_blPressed)
    {
        QWidget * widget = qobject_cast<QWidget *>(obj);
        // 确保事件发生在本窗口及其子控件上
        if (nullptr != widget && this == widget->window())
        {
            QMouseEvent * ev = static_cast<QMouseEvent *>(event);
            QPoint pos = this->mapFromGlobal(ev->globalPos());
            updateCursorShape(pos);
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
            m_pVolumeWidget->hideVolume();
            return true;
        }
        else if (QEvent::Wheel == event->type())
        {
            QWheelEvent * wheel = static_cast<QWheelEvent *>(event);
            //if (!m_isMuted) {
            int delta = wheel->angleDelta().y();
            int step = (delta > 0) ? 1 : -1;
            int current = m_pVolumeWidget->getVolume();
            int vol = qBound(0, current + step, 128);

            m_pVolumeWidget->setVolume(vol);
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
    ui.m_widgetMenu->setText(QStringLiteral("%1").arg(fileInfo.fileName()));

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
    auto val = m_pVolumeWidget->getVolume();
    if (val > 0)
    {
        m_pVolumeWidget->setVolume(0);
        CXPlayerSource::uniqueInstance().setVolume(0);
        ui.m_btnVolume->setIcon(QIcon(":/XPlayer/res/silence.ico"));
    }
    else
    {
        m_pVolumeWidget->setVolume(50);
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
    ui.m_widgetMenu->setText(QStringLiteral("%1").arg(fileInfo.fileName()));

    m_pVodWidget->addRecord(fileInfo.fileName(), strFileName);

    play(strFileName.toStdString());
}

void XPlayer::onBtnClickedLive()
{
    std::vector<QString> urls;
    m_pLiveWidget->getRecord(urls);

    CLinkWidget lw(this);
    lw.setRecord(urls);
    auto ret = lw.exec();
    if (QDialog::Accepted != ret)
        return;

    cleanup();

    auto url = lw.getUrl();
    ui.m_widgetMenu->setText(url);

    m_pLiveWidget->addRecord(url, url);

    play(url.toStdString());
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
    if (!m_pVolumeWidget->isVisible())
    {
        QRect rect = ui.m_btnVolume->rect();
        QPoint tlr = ui.m_btnVolume->mapToGlobal(rect.topLeft());
        int height = m_pVolumeWidget->height();
        QPoint pos(tlr.x(), tlr.y() - height);
        m_pVolumeWidget->showVolume(pos);
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
    ui.m_widgetMenu->setText(item->text());
    play(path.toStdString());
}

void XPlayer::onVideoRendererTriggered(QAction * action)
{
    auto mode = static_cast<XPLAYER_VIDEO_RENDERER_TYPE>(action->data().toInt());
    CXPlayerSource::uniqueInstance().selectVideoRenderer(mode);
}

void XPlayer::onVideoTracksTriggered(QAction * action)
{
    auto index = action->data().toInt();
    CXPlayerSource::uniqueInstance().selectStream(index, true);
}

void XPlayer::onAudioTracksTriggered(QAction * action)
{
    auto index = action->data().toInt();
    CXPlayerSource::uniqueInstance().selectStream(index, false);
}

void XPlayer::onFrameReady()
{
    ui.m_wndScreen->repaint();
}

void XPlayer::moveCenter()
{
    QScreen * screen = QGuiApplication::primaryScreen();
    auto screen_rect = screen->geometry();
    auto cx = screen_rect.width() / 2;
    auto cy = screen_rect.height() / 2;
    auto rect = this->frameGeometry();
    auto dx = cx - rect.width() / 2;
    auto dy = cy - rect.height() / 2;
    move(dx, dy);
}

void XPlayer::initMenuBar()
{
    ui.m_actDisableAudio->setCheckable(true);
    ui.m_actDisableVideo->setCheckable(true);
    ui.m_actDisableAudio->setData(QVariant::fromValue(-1));
    ui.m_actDisableVideo->setData(QVariant::fromValue(-1));

    ui.m_actVod->setIcon(QIcon(":/XPlayer/res/vod.ico"));
    ui.m_actLive->setIcon(QIcon(":/XPlayer/res/live.ico"));

    ui.m_actVod->setShortcut(QKeySequence::Open);
    ui.m_actLive->setShortcut(QKeySequence::Underline);

    auto * open_menu = ui.m_widgetMenu->addMenu(QStringLiteral("媒体"));
    open_menu->addAction(ui.m_actVod);
    open_menu->addAction(ui.m_actLive);

    auto * video_menu = ui.m_widgetMenu->addMenu(QStringLiteral("视频"));

    // 视频渲染器
    m_pmnuVideoRenderers = video_menu->addMenu(QStringLiteral("渲染器"));
    m_grpVideoRenderers = new QActionGroup(this);
    auto * act = m_pmnuVideoRenderers->addAction(QStringLiteral("SDL2"));
    act->setCheckable(true);
    act->setChecked(true);
    act->setData(QVariant::fromValue((int)XPLAYER_VIDEO_RENDERER_SDL2));
    m_grpVideoRenderers->addAction(act);
    act = m_pmnuVideoRenderers->addAction(QStringLiteral("OpenGL"));
    act->setCheckable(true);
    act->setChecked(false);
    act->setData(QVariant::fromValue((int)XPLAYER_VIDEO_RENDERER_OPENGL));
    m_grpVideoRenderers->addAction(act);
    m_grpVideoRenderers->setExclusive(true);
    connect(m_grpVideoRenderers, &QActionGroup::triggered, this, &XPlayer::onVideoRendererTriggered);

    // 视频轨道
    m_pmnuVideoTracks = video_menu->addMenu(QStringLiteral("轨道"));
    m_pmnuVideoTracks->addAction(ui.m_actDisableVideo);

    m_pmnuAudioTracks = ui.m_widgetMenu->addMenu(QStringLiteral("音频"));
    m_pmnuAudioTracks->addAction(ui.m_actDisableAudio);

    m_grpVideoTracks = new QActionGroup(this);
    m_grpVideoTracks->setExclusive(true);
    m_grpVideoTracks->addAction(ui.m_actDisableVideo);
    connect(m_grpVideoTracks, &QActionGroup::triggered, this, &XPlayer::onVideoTracksTriggered);

    m_grpAudioTracks = new QActionGroup(this);
    m_grpAudioTracks->setExclusive(true);
    m_grpAudioTracks->addAction(ui.m_actDisableAudio);
    connect(m_grpAudioTracks, &QActionGroup::triggered, this, &XPlayer::onAudioTracksTriggered);

    connect(ui.m_actVod, &QAction::triggered, this, &XPlayer::onBtnClickedVod);
    connect(ui.m_actLive, &QAction::triggered, this, &XPlayer::onBtnClickedLive);

    connect(ui.m_widgetMenu, &CMenuWidget::minimizeClicked, this, &XPlayer::onBtnClickedMinimize);
    connect(ui.m_widgetMenu, &CMenuWidget::maximizeClicked, this, &XPlayer::onBtnClickedMaximize);
    connect(ui.m_widgetMenu, &CMenuWidget::closeClicked, this, &XPlayer::onBtnClickedClose);
}

void XPlayer::updateCursorShape(const QPoint & pt)
{
    // 定义一个“忽略列表”, QPushButton加进去
    QWidget * target_widget = QApplication::widgetAt(QCursor::pos());
    if (nullptr != target_widget)
    {
        QString strClassName = QString::fromLatin1(target_widget->metaObject()->className());
        QStringList ignoreList = {
            "QMenuBar",
            "QMenu",
            "QComboBox",
            "QListWidget",
            "QTableWidget",
            "QPushButton"
        };
        if (ignoreList.contains(strClassName))
        {
            setCursor(Qt::ArrowCursor);
            return;
        }
    }

    const int edge_width = 10;

    int x = pt.x();
    int y = pt.y();
    int w = width();
    int h = height();

    m_uiEdge = 0u;
    // 左侧
    if (x < edge_width)
        m_uiEdge |= Qt::Edge::LeftEdge;
    // 上侧
    if (y < edge_width)
        m_uiEdge |= Qt::Edge::TopEdge;
    // 右侧
    if (w - edge_width < x)
        m_uiEdge |= Qt::Edge::RightEdge;
    // 下侧
    if (h - edge_width < y)
        m_uiEdge |= Qt::Edge::BottomEdge;

    // 变换形状
    switch (m_uiEdge)
    {
    case Qt::Edge::LeftEdge:
    case Qt::Edge::RightEdge:
        setCursor(Qt::SizeHorCursor);
        break;
    case Qt::Edge::TopEdge:
    case Qt::Edge::BottomEdge:
        setCursor(Qt::SizeVerCursor);
        break;
    case (Qt::Edge::LeftEdge | Qt::Edge::TopEdge):
    case (Qt::Edge::RightEdge | Qt::Edge::BottomEdge):
        setCursor(Qt::SizeFDiagCursor);
        break;
    case (Qt::Edge::LeftEdge | Qt::Edge::BottomEdge):
    case (Qt::Edge::RightEdge | Qt::Edge::TopEdge):
        setCursor(Qt::SizeBDiagCursor);
        break;
    default:
        setCursor(Qt::ArrowCursor);
        break;
    }
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
        act->setObjectName(QStringLiteral("m_actAudio%1").arg(index));
        m_pmnuAudioTracks->addAction(act);
        m_grpAudioTracks->addAction(act);
    }

    for (auto iter = vis.cbegin(); iter != vis.cend(); iter++)
    {
        const auto index = static_cast<int>(std::distance(vis.cbegin(), iter));
        auto * act = new QAction(this);
        act->setCheckable(true);
        act->setChecked(0 == index);
        act->setText(QStringLiteral("视轨%1").arg(index));
        act->setData(QVariant::fromValue(*iter));
        act->setObjectName(QStringLiteral("m_actVideo%1").arg(index));
        m_pmnuVideoTracks->addAction(act);
        m_grpVideoTracks->addAction(act);
    }

    // 为了解决SDL_DestroyWindow后画面显示问题
    ui.m_wndScreen->hide();
    ui.m_wndScreen->show();
    int width = 0, height = 0;
    getDisplaySize(width, height);
    if (!CXPlayerSource::uniqueInstance().play(ui.m_wndScreen, width, height))
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
    ui.m_widgetMenu->setText("");
    ui.m_btnCtrl->setIcon(QIcon(":/XPlayer/res/pause.ico"));
    ui.m_btnCtrl->setToolTip(QStringLiteral("播放"));
    ui.m_sldProgress->setValue(0);

    auto actions = m_grpVideoTracks->actions();
    for (auto & action : actions)
    {
        if (ui.m_actDisableVideo->objectName() == action->objectName())
            continue;
        m_grpVideoTracks->removeAction(action);
    }

    actions = m_grpAudioTracks->actions();
    for (auto & action : actions)
    {
        if (ui.m_actDisableAudio->objectName() == action->objectName())
            continue;
        m_grpAudioTracks->removeAction(action);
    }

    actions = m_pmnuAudioTracks->actions();
    for (auto & action : actions)
    {
        if (ui.m_actDisableAudio->objectName() == action->objectName())
            continue;

        m_pmnuAudioTracks->removeAction(action);
        if (nullptr == action->parent())
            delete action;
    }

    actions = m_pmnuVideoTracks->actions();
    for (auto & action : actions)
    {
        if (ui.m_actDisableVideo->objectName() == action->objectName())
            continue;

        m_pmnuVideoTracks->removeAction(action);
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
    auto strFontPath = QString::fromStdString(CXPlayerConfig::uniqueInstance().getFontPath());
    if (strFontPath.isEmpty())
    {
        strFontPath = path + QStringLiteral("/fonts/微软雅黑.ttc");
        CXPlayerConfig::uniqueInstance().setFontPath(strFontPath.toUtf8().toStdString());
    }

    m_pVodWidget->loadRecord(strVodPath);
    m_pLiveWidget->loadRecord(strLivePath);

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
