#pragma once

#include <QDialog>
#include "ui_CacheWidget.h"

class CacheWidget : public QDialog
{
    Q_OBJECT

public:
    CacheWidget(QWidget *parent = nullptr);
    ~CacheWidget();

public slots:
    void onCacheDurationChanged(int value);
    void onCacheFramesChanged(int value);

private:
    Ui::CacheWidgetClass ui;
};

