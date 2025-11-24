#include "mainwindow.h"

#include "qfiledialog.h"
#include "qmenu.h"
#include "qmenubar.h"

#include "numgeom/loadfromvtk.h"

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
        connect(act, SIGNAL(triggered()), this, SLOT(onOpenFile()));
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


void MainWindow::onOpenFile()
{
    QString filename = QFileDialog::getOpenFileName(
        this,
        tr("Select open file")
    );
    if(filename.isEmpty())
        return;

    auto mesh = LoadTriMeshFromVtk(filename.toStdString());
    std::cout << "Nodes: " << (mesh ? mesh->NbNodes() : 0) << std::endl;
    std::cout << "Cells: " << (mesh ? mesh->NbCells() : 0) << std::endl;
    m_sceneWidget->updateGeometry(mesh);
}
