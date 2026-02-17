#include "mainwindow.h"

#include <iostream>

#include "qapplication.h"
#include "qevent.h"
#include "qfiledialog.h"
#include "qmenu.h"
#include "qmenubar.h"
#include "qscreen.h"

#include "numgeom/application.h"
#include "numgeom/gpumanager.h"
#include "numgeom/loadfromvtk.h"

#include "scenewindow2.h"


MainWindow::MainWindow(Application* app)
{
    m_app = app;
    this->createActions();

    m_sceneWindow = new SceneWindow2(m_app);
    m_sceneWindow->setSurfaceType(QSurface::VulkanSurface);

    QWidget* widget = QWidget::createWindowContainer(m_sceneWindow, this);
    this->setCentralWidget(widget);
}


MainWindow::~MainWindow()
{
}


void MainWindow::initVulkan()
{
    GpuManager* gpuManager = m_app->gpuManager();
    m_vulkanInstance.setVkInstance(gpuManager->instance());
    if(!m_vulkanInstance.create())
        qFatal("Vulkan instance creating error");
    m_sceneWindow->setVulkanInstance(&m_vulkanInstance);

    VkSurfaceKHR surface = QVulkanInstance::surfaceForWindow(m_sceneWindow);
    assert(surface != VK_NULL_HANDLE);
    gpuManager->setSurface(surface);

    m_app->set_aspect_function(
        [this]() -> float {
            QSize sz = m_sceneWindow->size();
            qreal r = m_sceneWindow->devicePixelRatio();
            uint32_t width = static_cast<uint32_t>(sz.width() * r);
            uint32_t height = static_cast<uint32_t>(sz.height() * r);
            return width / static_cast<float>(height);
        }
    );
    gpuManager->setImageExtentFunction(
        [this]() -> std::tuple<uint32_t,uint32_t> {
            QSize sz = m_sceneWindow->size();
            qreal r = m_sceneWindow->devicePixelRatio();
            uint32_t width = static_cast<uint32_t>(sz.width() * r);
            uint32_t height = static_cast<uint32_t>(sz.height() * r);
            return std::make_tuple(width, height);
        }
    );

    gpuManager->initialize(); //< Продолжить начатую выше инициализацию.
    auto mesh = LoadTriMeshFromVtk("/home/tim/projects/numgeom/tests/data/polydata-cube.vtk");
    m_app->add(mesh);
    m_app->update();
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
        QAction* act = new QAction(icon, tr("Fit scene"), this);
        connect(act, SIGNAL(triggered()), this, SLOT(onFitScene()));
        act->setShortcut(QKeySequence(tr("F5")));
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
    m_app->add(mesh);
}


void MainWindow::onFitScene()
{
    m_app->fitScene();
}
