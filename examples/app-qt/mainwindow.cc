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
#include "numgeom/axisindicator.h"

#include "loadtotrimesh.h"
#include "scenewindow.h"

MainWindow::MainWindow(Application* app) {
  app_ = app;
  this->createActions();
  scene_window_ = new SceneWindow(app_);
  scene_window_->setSurfaceType(QSurface::VulkanSurface);
  QWidget* widget = QWidget::createWindowContainer(scene_window_, this);
  this->setCentralWidget(widget);
}

MainWindow::~MainWindow() {}

void MainWindow::initVulkan() {
  GpuManager* gpu_manager = app_->gpuManager();
  vulkan_instance_.setVkInstance(gpu_manager->instance());
  if (!vulkan_instance_.create()) qFatal("Vulkan instance creating error");
  scene_window_->setVulkanInstance(&vulkan_instance_);

  VkSurfaceKHR surface = QVulkanInstance::surfaceForWindow(scene_window_);
  assert(surface != VK_NULL_HANDLE);
  gpu_manager->setSurface(surface);

  app_->set_aspect_function([this]() -> float {
    QSize sz = scene_window_->size();
    qreal r = scene_window_->devicePixelRatio();
    uint32_t width = static_cast<uint32_t>(sz.width() * r);
    uint32_t height = static_cast<uint32_t>(sz.height() * r);
    return width / static_cast<float>(height);
  });
  gpu_manager->setImageExtentFunction(
      [this]() -> std::tuple<uint32_t, uint32_t> {
        QSize sz = scene_window_->size();
        qreal r = scene_window_->devicePixelRatio();
        uint32_t width = static_cast<uint32_t>(sz.width() * r);
        uint32_t height = static_cast<uint32_t>(sz.height() * r);
        return std::make_tuple(width, height);
      });

  gpu_manager->initialize();  //< Продолжить начатую выше инициализацию.
  auto mesh = GetAxisIndicatorMesh();
  app_->add(mesh);
  app_->update();
}

void MainWindow::createActions() {
  QMenu* file_menu = menuBar()->addMenu(tr("&File"));

  {
    QIcon icon;
    icon.addFile(QString::fromUtf8(":/resources/icons/openfile-32.png"),
                 QSize(), QIcon::Normal, QIcon::Off);
    QAction* act = new QAction(icon, tr("&Open"), this);
    act->setShortcuts(QKeySequence::Open);
    act->setStatusTip(tr("Open a file"));
    connect(act, SIGNAL(triggered()), this, SLOT(onOpenFile()));
    file_menu->addAction(act);
  }

  {
    QIcon icon;
    icon.addFile(QString::fromUtf8(":/resources/icons/screenshot-16.png"),
                 QSize(), QIcon::Normal, QIcon::Off);
    QAction* act = new QAction(icon, tr("Screenshot"), this);
    connect(act, SIGNAL(triggered()), this, SLOT(onScreenshot()));
    file_menu->addAction(act);
  }

  {
    QIcon icon;
    icon.addFile(QString::fromUtf8(":/resources/icons/closeapp-16.png"),
                 QSize(), QIcon::Normal, QIcon::Off);
    QAction* act = new QAction(icon, tr("Quit"), this);
    connect(act, SIGNAL(triggered()), this, SLOT(onQuit()));
    file_menu->addAction(act);
  }

  {
    QIcon icon;
    icon.addFile(QString::fromUtf8(":/resources/icons/fitscreen-16.png"),
                 QSize(), QIcon::Normal, QIcon::Off);
    QAction* act = new QAction(icon, tr("Fit scene"), this);
    connect(act, SIGNAL(triggered()), this, SLOT(onFitScene()));
    act->setShortcut(QKeySequence(tr("F5")));
    file_menu->addAction(act);
  }
}

void MainWindow::onScreenshot() {
  QString filename = "screen.png";
  QScreen* screen = QApplication::primaryScreen();
  QPixmap screenshot = screen->grabWindow(
      0, this->mapToGlobal(QPoint(0, 0)).x(),
      this->mapToGlobal(QPoint(0, 0)).y(), this->width(), this->height());
  screenshot.save(filename, "PNG");
}

void MainWindow::onQuit() {
    this->close();
}

void MainWindow::onOpenFile() {
  QString filename = QFileDialog::getOpenFileName(this, tr("Select open file"));
  if (filename.isEmpty()) return;

  auto mesh = LoadToTriMesh(filename.toStdString());
  std::cout << "Nodes: " << (mesh ? mesh->NbNodes() : 0) << std::endl;
  std::cout << "Cells: " << (mesh ? mesh->NbCells() : 0) << std::endl;
  app_->add(mesh);
}

void MainWindow::onFitScene() {
    app_->fitScene();
}
