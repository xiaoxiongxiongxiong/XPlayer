#include "CacheWidget.h"

#include "xplayer_config.h"

CacheWidget::CacheWidget(QWidget *parent)
    : QDialog(parent)
{
    ui.setupUi(this);

    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    connect(ui.m_spbDuration, QOverload<int>::of(&QSpinBox::valueChanged), this, &CacheWidget::onCacheDurationChanged);
    connect(ui.m_spbFrames, QOverload<int>::of(&QSpinBox::valueChanged), this, &CacheWidget::onCacheFramesChanged);
}

CacheWidget::~CacheWidget()
{}

void CacheWidget::onCacheDurationChanged(int value)
{
    CXPlayerConfig::uniqueInstance().set<xplayer_cache_duration_t>(value);
}

void CacheWidget::onCacheFramesChanged(int value)
{
    CXPlayerConfig::uniqueInstance().set<xplayer_cache_frame_t>(value);
}
