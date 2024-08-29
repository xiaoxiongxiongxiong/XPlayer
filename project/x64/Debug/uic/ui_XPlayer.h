/********************************************************************************
** Form generated from reading UI file 'XPlayer.ui'
**
** Created by: Qt User Interface Compiler version 5.14.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_XPLAYER_H
#define UI_XPLAYER_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_XPlayerClass
{
public:
    QMenuBar *menuBar;
    QToolBar *mainToolBar;
    QWidget *centralWidget;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *XPlayerClass)
    {
        if (XPlayerClass->objectName().isEmpty())
            XPlayerClass->setObjectName(QString::fromUtf8("XPlayerClass"));
        XPlayerClass->resize(600, 400);
        menuBar = new QMenuBar(XPlayerClass);
        menuBar->setObjectName(QString::fromUtf8("menuBar"));
        XPlayerClass->setMenuBar(menuBar);
        mainToolBar = new QToolBar(XPlayerClass);
        mainToolBar->setObjectName(QString::fromUtf8("mainToolBar"));
        XPlayerClass->addToolBar(mainToolBar);
        centralWidget = new QWidget(XPlayerClass);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        XPlayerClass->setCentralWidget(centralWidget);
        statusBar = new QStatusBar(XPlayerClass);
        statusBar->setObjectName(QString::fromUtf8("statusBar"));
        XPlayerClass->setStatusBar(statusBar);

        retranslateUi(XPlayerClass);

        QMetaObject::connectSlotsByName(XPlayerClass);
    } // setupUi

    void retranslateUi(QMainWindow *XPlayerClass)
    {
        XPlayerClass->setWindowTitle(QCoreApplication::translate("XPlayerClass", "XPlayer", nullptr));
    } // retranslateUi

};

namespace Ui {
    class XPlayerClass: public Ui_XPlayerClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_XPLAYER_H
