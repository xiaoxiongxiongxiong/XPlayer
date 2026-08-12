#include "FontWidget.h"

#include <QDir>
#include <QFileInfo>
#include <QMessageBox>

#include "xplayer_config.h"
#include "xplayer_source.h"

FontWidget::FontWidget(QWidget *parent)
    : QDialog(parent)
{
    ui.setupUi(this);

    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    traversePath();

    auto size = CXPlayerConfig::uniqueInstance().get<xplayer_font_size_t>();
    ui.m_spbSize->setValue(size);

    auto color = CXPlayerConfig::uniqueInstance().get<xplayer_font_color_t>();
    ui.m_sldRed->setValue(color.red);
    ui.m_sldGreen->setValue(color.green);
    ui.m_sldBlue->setValue(color.blue);
    ui.m_sldAlpha->setValue(color.alpha);

    showValue(color.red, color.green, color.blue, color.alpha, size);

    connect(ui.m_sldRed, &QSlider::valueChanged, this, &FontWidget::onSliderValueChanged);
    connect(ui.m_sldGreen, &QSlider::valueChanged, this, &FontWidget::onSliderValueChanged);
    connect(ui.m_sldBlue, &QSlider::valueChanged, this, &FontWidget::onSliderValueChanged);
    connect(ui.m_sldAlpha, &QSlider::valueChanged, this, &FontWidget::onSliderValueChanged);
    connect(ui.m_spbSize, QOverload<int>::of(&QSpinBox::valueChanged), this, &FontWidget::onSliderValueChanged);
    connect(ui.m_cboPath, SIGNAL(currentIndexChanged(int)), this, SLOT(onComboxSelected(int)));
}

FontWidget::~FontWidget()
{
}

void FontWidget::onSliderValueChanged(int value)
{
    xplayer_color_t color{};
    color.red = ui.m_sldRed->value();
    color.green = ui.m_sldGreen->value();
    color.blue = ui.m_sldBlue->value();
    color.alpha = ui.m_sldAlpha->value();

    auto size = ui.m_spbSize->value();

    showValue(color.red, color.green, color.blue, color.alpha, size);

    CXPlayerConfig::uniqueInstance().set<xplayer_font_color_t>(color);
    CXPlayerConfig::uniqueInstance().set<xplayer_font_size_t>(size);

    CXPlayerSource::uniqueInstance().setFontSize(size);
    CXPlayerSource::uniqueInstance().setFontColor(color);
}

void FontWidget::onComboxSelected(int index)
{
    auto path = ui.m_cboPath->itemData(index).toString();
    CXPlayerConfig::uniqueInstance().set<xplayer_font_path_t>(path.toStdString());
    CXPlayerSource::uniqueInstance().setFontPath(path.toStdString());
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

    auto font_path = QString::fromStdString(CXPlayerConfig::uniqueInstance().get<xplayer_font_path_t>());

    // 获取当前目录下的所有文件和文件夹，排除 . 和 ..
    QFileInfoList fonts = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    int index = 0;
    for (const QFileInfo & info : fonts)
    {
        ui.m_cboPath->addItem(info.fileName(), info.absoluteFilePath());
        if (font_path == info.absoluteFilePath())
            ui.m_cboPath->setCurrentIndex(index);
        index++;
    }

    if (font_path.isEmpty() && !fonts.isEmpty())
        ui.m_cboPath->setCurrentIndex(0);
}

void FontWidget::showValue(int r, int g, int b, int alpha, int size)
{
    auto a = static_cast<float>(alpha) / 100.0f;

    ui.m_labRedValue->setText(QString::number(r));
    ui.m_labGreenValue->setText(QString::number(g));
    ui.m_labBlueValue->setText(QString::number(b));
    ui.m_labAlphaValue->setText(QString::number(a, 'f', 2));

    auto str = QString::asprintf("color: rgba(%d, %d, %d, %.2f); font-size: %dpx;", r, g, b, a, size);
    ui.m_labSample->setStyleSheet(str);
}
