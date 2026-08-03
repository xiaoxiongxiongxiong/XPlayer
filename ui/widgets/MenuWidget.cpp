#include "MenuWidget.h"
#include <QMenuBar>
#include <QMenu>

CMenuWidget::CMenuWidget(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFixedHeight(30); // 与原生菜单栏高度一致
    // 创建菜单栏
    m_pMenuBar = new QMenuBar(this);
    m_pMenuBar->setMaximumWidth(240);

    ui.horizontalLayout->insertWidget(0, m_pMenuBar);

    connect(ui.m_btnMin, &QPushButton::clicked, this, &CMenuWidget::onBtnClickedMinimize);
    connect(ui.m_btnMax, &QPushButton::clicked, this, &CMenuWidget::onBtnClickedMaximize);
    connect(ui.m_btnClose, &QPushButton::clicked, this, &CMenuWidget::onBtnClickedClose);
}

CMenuWidget::~CMenuWidget()
{
    if (nullptr != m_pMenuBar)
    {
        delete m_pMenuBar;
        m_pMenuBar = nullptr;
    }
}

QMenu * CMenuWidget::addMenu(const QString & title)
{
    return m_pMenuBar->addMenu(title);
}

void CMenuWidget::setText(const QString & text)
{
    ui.m_labTitle->setText(text);
}

void CMenuWidget::onBtnClickedMinimize()
{
    emit minimizeClicked();
}

void CMenuWidget::onBtnClickedMaximize()
{
    emit maximizeClicked();
}

void CMenuWidget::onBtnClickedClose()
{
    emit closeClicked();
}
