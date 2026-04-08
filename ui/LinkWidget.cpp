#include "LinkWidget.h"
#include <QMessageBox>

CLinkWidget::CLinkWidget(QWidget *parent)
    : QDialog(parent)
{
    ui.setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    connect(ui.m_btnConfirm, SIGNAL(clicked()), this, SLOT(onBtnClickedConfirm()));
    connect(ui.m_btnCancel, SIGNAL(clicked()), this, SLOT(onBtnClickedCancel()));
    connect(ui.m_lstRecord, &QListWidget::itemClicked, this, &CLinkWidget::onItemClicked);
}

CLinkWidget::~CLinkWidget()
{}

void CLinkWidget::setRecord(const std::vector<QString> & urls)
{
    for (const auto & url : urls)
    {
        auto * item = new QListWidgetItem(url);
        //item->setData(Qt::UserRole + 1, QVariant::fromValue(url));
        ui.m_lstRecord->addItem(item);
    }
}

QString CLinkWidget::getUrl()
{
    return ui.m_edtUrl->toPlainText();
}

void CLinkWidget::onBtnClickedConfirm()
{
    if (ui.m_edtUrl->toPlainText().isEmpty())
    {
        QMessageBox::critical(this, QStringLiteral("警告"), QStringLiteral("链接为空"));
        return;
    }

    accept();
}

void CLinkWidget::onBtnClickedCancel()
{
    reject();
}

void CLinkWidget::onItemClicked(QListWidgetItem * item)
{
    auto url = item->text();
    ui.m_edtUrl->setText(url);
}
