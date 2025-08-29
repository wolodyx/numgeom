#include "mainwindow.h"

#include <QMenu>
#include <QMenuBar>
#include <QLabel>

#include "vulkanwidget.h"


MainWindow::MainWindow()
{
    //this->createActions();

    //auto centralWidget = new QLabel(tr("hello"));
    auto w = new VulkanWidget;
    this->setCentralWidget(w);
}


void MainWindow::createActions()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    const QIcon newIcon = QIcon::fromTheme("document-new", QIcon(":/images/new.png"));
    QAction *newAct = new QAction(newIcon, tr("&New"), this);
    newAct->setShortcuts(QKeySequence::New);
    newAct->setStatusTip(tr("Create a new file"));
    fileMenu->addAction(newAct);
}
