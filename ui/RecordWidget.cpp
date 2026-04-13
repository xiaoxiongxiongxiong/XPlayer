#include "RecordWidget.h"
#include <QMessageBox>
#include <QFileInfo>
#include <QFileDialog>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>

#include "LinkWidget.h"
#include "record/xplayer_record.h"

CRecordWidget::CRecordWidget(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);

    this->setAcceptDrops(true);

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

void CRecordWidget::getRecord(std::vector<QString> & urls)
{
    urls.clear();
    int cnt = ui.m_lstRecord->count();
    for (int i = 0; i < cnt; i++)
    {
        auto * item = ui.m_lstRecord->item(i);
        urls.push_back(item->text());
    }
}

void CRecordWidget::dragEnterEvent(QDragEnterEvent * event)
{
    if (!event->mimeData()->hasUrls())
    {
        event->ignore();
        return;
    }

    event->acceptProposedAction();
}

void CRecordWidget::dropEvent(QDropEvent * event)
{
    if (XPLAYER_MODE_VOD != m_iRecordMode)
        return;

    const QMimeData * mime_data = event->mimeData();
    if (!mime_data->hasUrls())
        return;

    QList<QUrl> urls = mime_data->urls();
    for (const auto & url : urls)
    {
        QString strFileName = url.toLocalFile();
        QFileInfo fileInfo(strFileName);
        addRecord(fileInfo.fileName(), strFileName);
    }
}

void CRecordWidget::onBtnClickedAdd()
{
    if (XPLAYER_MODE_LIVE == m_iRecordMode)
    {
        CLinkWidget lw(nullptr);
        auto ret = lw.exec();
        if (QDialog::Accepted != ret)
            return;

        auto url = lw.getUrl();
        addRecord(url, url);
        return;
    }

    const QString strFilter = tr("mp4(*.mp4);;mpegts(*.ts);;All Files(*.*)");
    QString strFilePath = QFileDialog::getOpenFileName(this, QStringLiteral("文件对话框"), "F:\\media", strFilter);
    if (strFilePath.isEmpty())
        return;

    QFileInfo fileInfo(strFilePath);
    addRecord(fileInfo.fileName(), strFilePath);
}

void CRecordWidget::onBtnClickedDelete()
{
    auto items = ui.m_lstRecord->selectedItems();
    if (items.empty())
        return;

    std::vector<int> rows;
    for (const auto & item : items)
    {
        CXPlayerRecordInfo ri;
        ri._name = item->text().toStdString();
        ri._path = item->data(Qt::UserRole + 1).toString().toStdString();
        ri._mode = static_cast<XPLAYER_MODE>(m_iRecordMode);
        m_ptrContext->delRecord(ri);
        rows.push_back(ui.m_lstRecord->row(item));
    }

    std::sort(rows.begin(), rows.end());
    for (auto iter = rows.rbegin(); iter != rows.rend(); iter++)
    {
        delete ui.m_lstRecord->takeItem(*iter);
    }
}

void CRecordWidget::onBtnClickedMode()
{

}

void CRecordWidget::onLstDbclickedRecord(QListWidgetItem * item)
{
    emit itemDbclicked(item);
}
