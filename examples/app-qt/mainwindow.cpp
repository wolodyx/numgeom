#include "mainwindow.h"

#include "qapplication.h"
#include "qevent.h"
#include "qfiledialog.h"
#include "qmenu.h"
#include "qmenubar.h"
#include "qscreen.h"

#include "numgeom/application.h"
#include "numgeom/loadfromvtk.h"

#include "scenewindow.h"


MainWindow::MainWindow(Application* app)
{
    m_app = app;
    this->createActions();

    m_vulkanInstance.setExtensions({"VK_KHR_surface", "VK_KHR_xcb_surface"});
    m_vulkanInstance.setLayers({"VK_LAYER_KHRONOS_validation"});
    if(!m_vulkanInstance.create())
        qFatal("Vulkan instance creating error");
    m_sceneWindow = new SceneWindow(m_app);
    m_sceneWindow->setVulkanInstance(&m_vulkanInstance);
    QWidget* widget = QWidget::createWindowContainer(m_sceneWindow, this);
    this->setCentralWidget(widget);
    m_app->connectWithWindow(m_sceneWindow);
}


MainWindow::~MainWindow()
{
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

    {
        QIcon icon;
        icon.addFile(
            QString::fromUtf8(":/resources/icons/fitscreen-16.png"),
            QSize(),
            QIcon::Normal,
            QIcon::Off
        );
        QAction* act = new QAction(icon, tr("Fit screen"), this);
        connect(act, SIGNAL(triggered()), this, SLOT(onFitScreen()));
        fileMenu->addAction(act);
    }
}


void MainWindow::onScreenshot()
{
    QString filename = "screen.png";
    QScreen* screen = QApplication::primaryScreen();
    QPixmap screenshot = screen->grabWindow(
        0,
        this->mapToGlobal(QPoint(0,0)).x(),
        this->mapToGlobal(QPoint(0,0)).y(),
        this->width(),
        this->height()
    );
    screenshot.save(filename, "PNG");
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
    m_sceneWindow->updateGeometry(mesh);
}


void MainWindow::onFitScreen()
{
    std::cout << "Fit screen" << std::endl;
}
