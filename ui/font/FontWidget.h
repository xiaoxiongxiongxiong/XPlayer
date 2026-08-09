#pragma once

#include <QDialog>
#include "ui_FontWidget.h"

class FontWidget : public QDialog
{
    Q_OBJECT

public:
    FontWidget(QWidget *parent = nullptr);
    ~FontWidget();

public:
    bool init();

    // 获取路径
    QString getPath();

    // 获取大小
    int getSize();

    // 获取颜色
    int getRed();
    int getGreen();
    int getBlue();

public slots:
    void onSliderValueChanged(int value);

private:
    // 获取所有字体文件
    void traversePath();

private:
    Ui::FontWidgetClass ui;
};

