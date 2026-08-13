#pragma once

#include <QDialog>
#include "ui_LinkWidget.h"

class CLinkWidget : public QDialog
{
    Q_OBJECT

public:
    CLinkWidget(QWidget *parent = nullptr);
    ~CLinkWidget();

    void setRecord(const std::vector<QString> & urls);

    QString getUrl();

private slots:
    void onBtnClickedConfirm();
    void onBtnClickedCancel();
    void onItemClicked(QListWidgetItem * item);

private:
    Ui::CLinkWidgetClass ui;
};

