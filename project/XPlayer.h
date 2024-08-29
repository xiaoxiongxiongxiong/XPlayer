#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_XPlayer.h"

class XPlayer : public QMainWindow
{
    Q_OBJECT

public:
    XPlayer(QWidget *parent = nullptr);
    ~XPlayer();

private:
    Ui::XPlayerClass ui;
};
