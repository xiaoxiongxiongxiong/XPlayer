#include "SpeedWidget.h"

#include <QPainter>
#include <QButtonGroup>
#include <QMouseEvent>
#include <QStyle>

#include "xplayer_config.h"
#include "xplayer_source.h"

CSpeedWidget::CSpeedWidget(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);

    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);

    setAttribute(Qt::WA_TranslucentBackground);

    ui.m_sldSpeed->installEventFilter(this);
    connect(ui.m_sldSpeed, &QSlider::valueChanged, this, &CSpeedWidget::onSliderValueChanged);

    auto speed = CXPlayerConfig::uniqueInstance().get<xplayer_speed_custom_t>();
    auto str = QString::number(speed, 'f', 1);
    ui.m_radCustom->setText(QStringLiteral("自定义 (%1)").arg(str));
    ui.m_radCustom->setProperty("speed", speed);

    ui.m_rad2Point0->setProperty("speed", 2.0f);
    ui.m_rad1Point5->setProperty("speed", 1.5f);
    ui.m_radNormal->setProperty("speed", 1.0f);
    ui.m_rad0Point7->setProperty("speed", 0.7f);
    ui.m_rad0Point5->setProperty("speed", 0.5f);

    m_grpSpeed = new QButtonGroup(this);
    m_grpSpeed->addButton(ui.m_radCustom, 0);
    m_grpSpeed->addButton(ui.m_rad2Point0, 1);
    m_grpSpeed->addButton(ui.m_rad1Point5, 2);
    m_grpSpeed->addButton(ui.m_radNormal, 3);
    m_grpSpeed->addButton(ui.m_rad0Point7, 4);
    m_grpSpeed->addButton(ui.m_rad0Point5, 5);
    m_grpSpeed->setExclusive(true);

    auto id = CXPlayerConfig::uniqueInstance().get<xplayer_speed_id_t>();
    auto * rad = m_grpSpeed->button(id);
    rad->setChecked(true);
    if (0 != id)
        ui.m_sldSpeed->setEnabled(false);

    connect(m_grpSpeed, SIGNAL(buttonToggled(int, bool)), this, SLOT(onRadioButtonToggled(int, bool)));
}

CSpeedWidget::~CSpeedWidget()
{
    if (nullptr != m_grpSpeed)
    {
        delete m_grpSpeed;
        m_grpSpeed = nullptr;
    }
}

void CSpeedWidget::onRadioButtonToggled(int id, bool checked)
{
    if (0 == id)
        ui.m_sldSpeed->setEnabled(checked);

    if (!checked)
        return;

    auto * rad = m_grpSpeed->button(id);
    auto speed = rad->property("speed").toFloat();
    CXPlayerSource::uniqueInstance().setSpeed(speed);
    CXPlayerConfig::uniqueInstance().set<xplayer_speed_realtime_t>(speed);
    CXPlayerConfig::uniqueInstance().set<xplayer_speed_id_t>(id);
}

void CSpeedWidget::onSliderValueChanged(int value)
{
    auto speed = static_cast<float>(value) / 100.0f * 1.5f + 0.5f;
    auto str = QString::number(speed, 'f', 1);
    ui.m_radCustom->setText(QStringLiteral("自定义 (%1)").arg(str));
    ui.m_radCustom->setProperty("speed", speed);
    CXPlayerSource::uniqueInstance().setSpeed(speed);
    CXPlayerConfig::uniqueInstance().set<xplayer_speed_realtime_t>(speed);
    CXPlayerConfig::uniqueInstance().set<xplayer_speed_custom_t>(speed);
}

void CSpeedWidget::paintEvent(QPaintEvent * event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPainterPath path;
    path.addRoundedRect(rect(), 12, 12);

    painter.fillPath(path, Qt::white);

    painter.setClipPath(path);

    QWidget::paintEvent(event);
}

bool CSpeedWidget::eventFilter(QObject * obj, QEvent * event)
{
    if (obj == ui.m_sldSpeed && !ui.m_sldSpeed->isEnabled())
        return QObject::eventFilter(obj, event);

    if (obj == ui.m_sldSpeed && (QEvent::MouseButtonPress == event->type() || QEvent::MouseButtonRelease == event->type()))
    {
        auto * ev = static_cast<QMouseEvent *>(event);
        if (Qt::LeftButton == ev->button())
        {
            int value = ui.m_sldSpeed->style()->sliderValueFromPosition(
                ui.m_sldSpeed->minimum(),
                ui.m_sldSpeed->maximum(),
                ev->pos().x() - 5,
                ui.m_sldSpeed->width() - 10
            );

            ui.m_sldSpeed->setValue(value);
            return true;
        }
    }

    return QObject::eventFilter(obj, event);
}
