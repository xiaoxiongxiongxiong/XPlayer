#pragma once

#include <QWidget>
#include "ui_MenuWidget.h"

class QMenuBar;
class QMenu;

class CMenuWidget : public QWidget
{
    Q_OBJECT

public:
    CMenuWidget(QWidget *parent = nullptr);
    ~CMenuWidget();

    QMenu * addMenu(const QString & title);

signals:
    void minimizeClicked();
    void maximizeClicked();
    void closeClicked();

private slots:
    void onBtnClickedMinimize();
    void onBtnClickedMaximize();
    void onBtnClickedClose();

private:
    Ui::CMenuWidgetClass ui;

    // 菜单栏
    QMenuBar * m_pMenuBar = nullptr;
};

