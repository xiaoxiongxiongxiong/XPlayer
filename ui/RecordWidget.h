#pragma once

#include <QWidget>
#include "ui_RecordWidget.h"

class CXPlayerRecord;

class CRecordWidget : public QWidget
{
    Q_OBJECT

public:
    CRecordWidget(QWidget *parent = nullptr);
    ~CRecordWidget();

    // 加载播放记录
    bool loadRecord(const QString & path);
    // 卸载播放记录
    void unloadRecord();

    // 上一条记录
    void lastRecord();
    // 下一条记录
    void nextRecord();

    // 添加播放记录
    bool addRecord(const QString & name, const QString & path);

    // 获取记录
    void getRecord(std::vector<QString> & urls);

signals:
    void itemDbclicked(QListWidgetItem * item);

private slots:
    void onBtnClickedAdd();
    void onBtnClickedDelete();
    void onBtnClickedMode();

    void onLstDbclickedRecord(QListWidgetItem * item);

private:
    Ui::CRecordWidgetClass ui;

    // 记录实例
    CXPlayerRecord * m_ptrContext = nullptr;

    // 记录类型
    int m_iRecordMode = -1;
};

