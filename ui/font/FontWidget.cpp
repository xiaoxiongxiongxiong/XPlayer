#include "FontWidget.h"

#include <QDir>
#include <QFileInfo>
#include <QMessageBox>

#include "xplayer_config.h"

FontWidget::FontWidget(QWidget *parent)
    : QDialog(parent)
{
    ui.setupUi(this);

    //ui.m_sldSpeed->installEventFilter(this);
    connect(ui.m_sldRed, &QSlider::valueChanged, this, &FontWidget::onSliderValueChanged);
    connect(ui.m_sldGreen, &QSlider::valueChanged, this, &FontWidget::onSliderValueChanged);
    connect(ui.m_sldBlue, &QSlider::valueChanged, this, &FontWidget::onSliderValueChanged);
    connect(ui.m_spbSize, QOverload<int>::of(&QSpinBox::valueChanged), this, &FontWidget::onSliderValueChanged);
}

FontWidget::~FontWidget()
{
}

bool FontWidget::init()
{
    traversePath();

    auto size = CXPlayerConfig::uniqueInstance().get<xplayer_font_size_t>();
    ui.m_spbSize->setValue(size);

    auto color = CXPlayerConfig::uniqueInstance().get<xplayer_font_color_t>();
    ui.m_sldRed->setValue(color.red);
    ui.m_sldGreen->setValue(color.green);
    ui.m_sldBlue->setValue(color.blue);

    auto str = QString::asprintf("color: rgb(%d, %d, %d); font-size: %dpx;", color.red, color.green, color.blue, size);
    ui.m_labSample->setStyleSheet(str);

    return true;
}

QString FontWidget::getPath()
{
    const auto index = ui.m_cboPath->currentIndex();
    auto path = ui.m_cboPath->itemData(index).toString();
    return path;
}

int FontWidget::getSize()
{
    return ui.m_spbSize->value();
}

int FontWidget::getRed()
{
    return ui.m_sldRed->value();
}

int FontWidget::getGreen()
{
    return ui.m_sldGreen->value();
}

int FontWidget::getBlue()
{
    return ui.m_sldBlue->value();
}

void FontWidget::onSliderValueChanged(int value)
{
    xplayer_color_t color{};
    color.red = ui.m_sldRed->value();
    color.green = ui.m_sldGreen->value();
    color.blue = ui.m_sldBlue->value();

    auto size = ui.m_spbSize->value();

    auto str = QString::asprintf("color: rgb(%d, %d, %d); font-size: %dpx;", color.red, color.green, color.blue, size);
    ui.m_labSample->setStyleSheet(str);

    CXPlayerConfig::uniqueInstance().set<xplayer_font_color_t>(color);
    CXPlayerConfig::uniqueInstance().set<xplayer_font_size_t>(size);
}

void FontWidget::traversePath()
{
    auto path = QCoreApplication::applicationDirPath() + "/fonts";

    QDir dir(path);
    if (!dir.exists())
    {
        QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("目录不存在"));
        return;
    }

    // 获取当前目录下的所有文件和文件夹，排除 . 和 ..
    QFileInfoList list = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    for (const QFileInfo & info : list)
    {
        ui.m_cboPath->addItem(info.fileName(), info.absoluteFilePath());
    }
}
