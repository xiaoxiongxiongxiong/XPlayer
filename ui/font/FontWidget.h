#pragma once

#include <QDialog>
#include "ui_FontWidget.h"

class FontWidget : public QDialog
{
    Q_OBJECT

public:
    FontWidget(QWidget *parent = nullptr);
    ~FontWidget();

private:
    Ui::FontWidgetClass ui;
};

