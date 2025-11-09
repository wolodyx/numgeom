#include "mainwindow.h"

#include "qmenu.h"
#include "qmenubar.h"

#include "vulkanwidget.h"


MainWindow::MainWindow()
{
    this->createActions();

    m_sceneWidget = new VulkanWidget;
    this->setCentralWidget(m_sceneWidget);
}


void MainWindow::createActions()
{
    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));

    {
        QIcon icon;
        icon.addFile(
            QString::fromUtf8(":/resources/icons/openfile-32.png"),
            QSize(),
            QIcon::Normal,
            QIcon::Off
        );
        QAction* act = new QAction(icon, tr("&Open"), this);
        act->setShortcuts(QKeySequence::Open);
        act->setStatusTip(tr("Open a file"));
        fileMenu->addAction(act);
    }

    {
        QIcon icon;
        icon.addFile(
            QString::fromUtf8(":/resources/icons/screenshot-16.png"),
            QSize(),
            QIcon::Normal,
            QIcon::Off
        );
        QAction* act = new QAction(icon, tr("Screenshot"), this);
        connect(act, SIGNAL(triggered()), this, SLOT(onScreenshot()));
        fileMenu->addAction(act);
    }

    {
        QIcon icon;
        icon.addFile(
            QString::fromUtf8(":/resources/icons/closeapp-16.png"),
            QSize(),
            QIcon::Normal,
            QIcon::Off
        );
        QAction* act = new QAction(icon, tr("Quit"), this);
        connect(act, SIGNAL(triggered()), this, SLOT(onQuit()));
        fileMenu->addAction(act);
    }
}


void MainWindow::onScreenshot()
{
    QString filename = "screen.png";
    m_sceneWidget->saveAsPng(filename);
}


void MainWindow::onQuit()
{
    this->close();
}
