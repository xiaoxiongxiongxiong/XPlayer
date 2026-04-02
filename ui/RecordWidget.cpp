#include "RecordWidget.h"
#include <QMessageBox>
#include <QFileInfo>
#include "record/xplayer_record.h"

CRecordWidget::CRecordWidget(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);

    connect(ui.m_btnAdd, SIGNAL(clicked()), this, SLOT(onBtnClickedAdd()));
    connect(ui.m_btnDelete, SIGNAL(clicked()), this, SLOT(onBtnClickedDelete()));
    connect(ui.m_btnMode, SIGNAL(clicked()), this, SLOT(onBtnClickedMode()));
    connect(ui.m_lstRecord, &QListWidget::itemDoubleClicked, this, &CRecordWidget::onLstDbclickedRecord);
}

CRecordWidget::~CRecordWidget()
{
    unloadRecord();
}

bool CRecordWidget::loadRecord(const QString & path)
{
    QFileInfo fileInfo(path);
    if ("vod.json" == fileInfo.fileName())
        m_iRecordMode = XPLAYER_MODE_VOD;
    else if ("live.json" == fileInfo.fileName())
        m_iRecordMode = XPLAYER_MODE_LIVE;
    else
    {
        QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("不支持的记录文件：%1！").arg(fileInfo.fileName()));
        return false;
    }

    m_ptrContext = new(std::nothrow) CXPlayerRecord();
    if (nullptr == m_ptrContext)
    {
        QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("加载点播记录失败！"));
        return false;
    }

    if (!m_ptrContext->loadRecordFile(path.toLocal8Bit().toStdString()))
    {
        QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("加载点播记录失败！"));
        delete m_ptrContext;
        m_ptrContext = nullptr;
    }

    std::vector<CXPlayerRecordInfo> elems;
    m_ptrContext->getRecordList(elems);
    for (const auto & elem : elems)
    {
        auto * item = new QListWidgetItem(QString::fromStdString(elem._name));
        item->setData(Qt::UserRole + 1, QVariant::fromValue(QString::fromStdString(elem._path)));
        ui.m_lstRecord->addItem(item);
    }

    return true;
}

void CRecordWidget::unloadRecord()
{
    if (nullptr != m_ptrContext)
    {
        m_ptrContext->unloadRecordFile();
        delete m_ptrContext;
        m_ptrContext = nullptr;
    }

    m_iRecordMode = -1;
}

void CRecordWidget::lastRecord()
{
    auto rows = ui.m_lstRecord->count();
    auto row = ui.m_lstRecord->currentRow();
    if (row <= 0)
        row = rows - 1;
    else
        row--;
    ui.m_lstRecord->setCurrentRow(row);
    auto * item = ui.m_lstRecord->currentItem();
    onLstDbclickedRecord(item);
}

void CRecordWidget::nextRecord()
{
    auto rows = ui.m_lstRecord->count();
    auto row = ui.m_lstRecord->currentRow();
    if (row + 1 >= rows)
        row = 0;
    else
        row++;
    ui.m_lstRecord->setCurrentRow(row);
    auto * item = ui.m_lstRecord->currentItem();
    onLstDbclickedRecord(item);
}

bool CRecordWidget::addRecord(const QString & name, const QString & path)
{
    if (nullptr == m_ptrContext)
    {
        return false;
    }

    CXPlayerRecordInfo pi;
    pi._mode = static_cast<XPLAYER_MODE>(m_iRecordMode);
    pi._name = name.toStdString();
    pi._path = path.toStdString();
    if (!m_ptrContext->addRecord(pi))
    {
        return false;
    }

    auto * item = new QListWidgetItem(name);
    item->setData(Qt::UserRole + 1, QVariant::fromValue(path));
    ui.m_lstRecord->addItem(item);
    ui.m_lstRecord->setCurrentRow(ui.m_lstRecord->count() - 1);

    return true;
}

void CRecordWidget::onBtnClickedAdd()
{

}

void CRecordWidget::onBtnClickedDelete()
{

}

void CRecordWidget::onBtnClickedMode()
{

}

void CRecordWidget::onLstDbclickedRecord(QListWidgetItem * item)
{
    emit itemDbclicked(item);
}
