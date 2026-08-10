#pragma once

#include <QDialog>
#include "ui_FontWidget.h"

class FontWidget : public QDialog
{
    Q_OBJECT

public:
    FontWidget(QWidget *parent = nullptr);
    ~FontWidget();

public slots:
    void onSliderValueChanged(int value);
    void onComboxSelected(int index);

private:
    // 获取所有字体文件
    void traversePath();

    // 显示输出
    void showValue(int r, int g, int b, int alpha, int size);

private:
    Ui::FontWidgetClass ui;
};

