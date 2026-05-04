#include "mainwindow.h"

#include <iostream>

#include "qapplication.h"
#include "qdir.h"
#include "qevent.h"
#include "qfile.h"
#include "qfiledialog.h"
#include "qfileinfo.h"
#include "qmenu.h"
#include "qmenubar.h"
#include "qresource.h"
#include "qscreen.h"

#include "numgeom/application.h"
#include "numgeom/gpumanager.h"
#include "numgeom/scene.h"
#include "numgeom/sceneobject_mesh.h"
#include "numgeom/scenewidget_axisindicator.h"
#ifdef USE_NUMGEOM_MODULE_OCC
#  include "numgeom/loadusingocc.h"
#  include "numgeom/sceneobject_polytriangulation.h"
#  include "numgeom/sceneobject_tdocstd_document.h"
#endif

#include "loadtotrimesh.h"
#include "scenewindow.h"

MainWindow::MainWindow(Application* app) : settings_("NumGeom", "QtDemo") {
  app_ = app;
  loadRecentFiles();
  this->createActions();
  scene_window_ = new SceneWindow(app_);
  scene_window_->setSurfaceType(QSurface::VulkanSurface);
  QWidget* widget = QWidget::createWindowContainer(scene_window_, this);
  this->setCentralWidget(widget);

  QResource resource(":/logo.jpg");
  assert(resource.isValid());
  app_->SetLogo(resource.data(), resource.size(), glm::ivec2(0,0));
}

MainWindow::~MainWindow() {}

void MainWindow::initVulkan() {
  GpuManager* gpu_manager = app_->GetGpuManager();
  vulkan_instance_.setVkInstance(gpu_manager->instance());
  if (!vulkan_instance_.create()) qFatal("Vulkan instance creating error");
  scene_window_->setVulkanInstance(&vulkan_instance_);

  VkSurfaceKHR surface = QVulkanInstance::surfaceForWindow(scene_window_);
  assert(surface != VK_NULL_HANDLE);
  gpu_manager->setSurface(surface);

  app_->SetViewportSizeFunction([this]() {
    QSize sz = scene_window_->size();
    qreal r = scene_window_->devicePixelRatio();
    uint32_t width = static_cast<uint32_t>(sz.width() * r);
    uint32_t height = static_cast<uint32_t>(sz.height() * r);
    return std::make_tuple(width, height);
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

  app_->GetScene().AddObject<SceneWidget_AxisIndicator>();
  app_->FitScene();
}

void MainWindow::createActions() {
  this->createFileMenu();
  this->createViewMenu();
}

void MainWindow::createFileMenu() {
  QMenu* menu = menuBar()->addMenu(tr("&File"));

  {
    QIcon icon;
    icon.addFile(QString::fromUtf8(":/resources/icons/openfile-32.png"),
                 QSize(), QIcon::Normal, QIcon::Off);
    QAction* act = new QAction(icon, tr("&Open"), this);
    act->setShortcuts(QKeySequence::Open);
    act->setStatusTip(tr("Open a file"));
    connect(act, SIGNAL(triggered()), this, SLOT(onOpenFile()));
    menu->addAction(act);
  }

  menu->addSeparator();

  recent_files_menu_ = menu->addMenu(tr("Open &Recent"));
  updateRecentFilesMenu();

  menu->addSeparator();

  {
    QIcon icon;
    icon.addFile(QString::fromUtf8(":/resources/icons/screenshot-16.png"),
                 QSize(), QIcon::Normal, QIcon::Off);
    QAction* act = new QAction(icon, tr("Screenshot"), this);
    connect(act, SIGNAL(triggered()), this, SLOT(onScreenshot()));
    menu->addAction(act);
  }

  {
    QIcon icon;
    icon.addFile(QString::fromUtf8(":/resources/icons/closeapp-16.png"),
                 QSize(), QIcon::Normal, QIcon::Off);
    QAction* act = new QAction(icon, tr("Quit"), this);
    connect(act, SIGNAL(triggered()), this, SLOT(onQuit()));
    menu->addAction(act);
  }
}

void MainWindow::createViewMenu() {
  QMenu* menu = menuBar()->addMenu(tr("&View"));

  {
    QIcon icon;
    icon.addFile(QString::fromUtf8(":/resources/icons/fitscreen-16.png"),
                 QSize(), QIcon::Normal, QIcon::Off);
    QAction* act = new QAction(icon, tr("Fit scene"), this);
    connect(act, SIGNAL(triggered()), this, SLOT(onFitScene()));
    act->setShortcut(QKeySequence(tr("F5")));
    menu->addAction(act);
  }

  QMenu* std_view = menu->addMenu(tr("Standard views"));
  {
    QAction* act = new QAction(tr("Front"), this);
    connect(act, &QAction::triggered,
            [=](){
              auto ortho_basis = OrthoBasis<float>::FromZY(
                  glm::vec3(0,1,0),
                  glm::vec3(0,0,1));
              app_->OrientCamera(ortho_basis);
            });
    act->setShortcut(QKeySequence(tr("1")));
    std_view->addAction(act);
  }
  {
    QAction* act = new QAction(tr("Top"), this);
    connect(act, &QAction::triggered,
            [=](){
              auto ortho_basis = OrthoBasis<float>::FromZY(
                  glm::vec3(0,0,-1),
                  glm::vec3(0,1,0));
              app_->OrientCamera(ortho_basis);
            });
    act->setShortcut(QKeySequence(tr("2")));
    std_view->addAction(act);
  }
  {
    QAction* act = new QAction(tr("Right"), this);
    connect(act, &QAction::triggered,
            [=](){
              auto ortho_basis = OrthoBasis<float>::FromZY(
                  glm::vec3(-1,0,0),
                  glm::vec3(0,0,1));
              app_->OrientCamera(ortho_basis);
            });
    act->setShortcut(QKeySequence(tr("3")));
    std_view->addAction(act);
  }
  {
    QAction* act = new QAction(tr("Rear"), this);
    connect(act, &QAction::triggered,
            [=](){
              auto ortho_basis = OrthoBasis<float>::FromZY(
                  glm::vec3(0,-1,0),
                  glm::vec3(0,0,1));
              app_->OrientCamera(ortho_basis);
            });
    act->setShortcut(QKeySequence(tr("4")));
    std_view->addAction(act);
  }
  {
    QAction* act = new QAction(tr("Bottom"), this);
    connect(act, &QAction::triggered,
            [=](){
              auto ortho_basis = OrthoBasis<float>::FromZY(
                  glm::vec3(0,0,1),
                  glm::vec3(0,-1,0));
              app_->OrientCamera(ortho_basis);
            });
    act->setShortcut(QKeySequence(tr("5")));
    std_view->addAction(act);
  }
  {
    QAction* act = new QAction(tr("Left"), this);
    connect(act, &QAction::triggered,
            [=](){
              auto ortho_basis = OrthoBasis<float>::FromZY(
                  glm::vec3(1,0,0),
                  glm::vec3(0,0,1));
              app_->OrientCamera(ortho_basis);
            });
    act->setShortcut(QKeySequence(tr("6")));
    std_view->addAction(act);
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

namespace {
bool IsStepFile(const QString& filename) {
  QFileInfo fileInfo(filename);
  QString suffix = fileInfo.suffix();
  suffix = suffix.toLower();
  return suffix == "step" || suffix == "stp";
}

bool IsStlFile(const QString& filename) {
  QFileInfo fileInfo(filename);
  QString suffix = fileInfo.suffix();
  suffix = suffix.toLower();
  return suffix == "stl";
}

bool IsIgesFile(const QString& filename) {
  QFileInfo fileInfo(filename);
  QString suffix = fileInfo.suffix();
  suffix = suffix.toLower();
  return suffix == "iges" || suffix == "igs";
}

bool IsObjFile(const QString& filename) {
  QFileInfo fileInfo(filename);
  QString suffix = fileInfo.suffix();
  suffix = suffix.toLower();
  return suffix == "obj";
}

bool IsVrmlFile(const QString& filename) {
  QFileInfo fileInfo(filename);
  QString suffix = fileInfo.suffix();
  suffix = suffix.toLower();
  return suffix == "wrl";
}

bool IsGltfFile(const QString& filename) {
  QFileInfo fileInfo(filename);
  QString suffix = fileInfo.suffix();
  suffix = suffix.toLower();
  return suffix == "glb";
}
}

void MainWindow::openFile(const QString& filename) {
  Scene& scene = app_->GetScene();
  scene.Clear();
#ifdef USE_NUMGEOM_MODULE_OCC
  if (IsStepFile(filename)) {
    auto document = LoadStepDocument(filename.toStdWString());
    if (!document) {
      qDebug() << "Ошибка при загрузке файла '" << filename << "'";
      return;
    }
    scene.AddObject<SceneObject_TDocStd_Document>(document);
    app_->FitScene();
    return;
  } else if (IsStlFile(filename)) {
    auto triangulation = LoadStl(filename.toStdWString());
    if (!triangulation) {
      qDebug() << "Ошибка при загрузке файла `" << filename << "'";
      return;
    }
    scene.AddObject<SceneObject_PolyTriangulation>(triangulation);
    app_->FitScene();
    return;
  } else if (IsIgesFile(filename)) {
    auto document = LoadIges(filename.toStdWString());
    if (!document) {
      qDebug() << "Ошибка при загрузке файла `" << filename << "'";
      return;
    }
    scene.AddObject<SceneObject_TDocStd_Document>(document);
    app_->FitScene();
    return;
  } else if (IsObjFile(filename)) {
    auto triangulation = LoadObj(filename.toStdWString());
    if (!triangulation) {
      qDebug() << "Ошибка при загрузке файла `" << filename << "'";
      return;
    }
    scene.AddObject<SceneObject_PolyTriangulation>(triangulation);
    app_->FitScene();
    return;
#ifdef NUMGEOM_OCC_LOAD_VRML
  } else if (IsVrmlFile(filename)) {
    auto document = LoadVrml(filename.toStdWString());
    if (!document) {
      qDebug() << "Ошибка при загрузке файла `" << filename << "'";
      return;
    }
    scene.AddObject<SceneObject_TDocStd_Document>(document);
    app_->FitScene();
    return;
#endif
  } else if (IsGltfFile(filename)) {
    auto document = LoadGltf(filename.toStdWString());
    if (!document) {
      qDebug() << "Ошибка при загрузке файла `" << filename << "'";
      return;
    }
    scene.AddObject<SceneObject_TDocStd_Document>(document);
    app_->FitScene();
    return;
  }
#endif
  auto mesh = LoadToTriMesh(filename.toStdWString());
  if(mesh) {
    scene.AddObject<SceneObject_Mesh>(mesh);
    app_->FitScene();
  }
}

void MainWindow::onOpenFile() {
  QString last_directory =
    settings_.value("MainWindow/lastDirectory", QDir::homePath()).toString();
  QString filename = QFileDialog::getOpenFileName(this, tr("Select open file"),
                                                  last_directory);
  if (filename.isEmpty()) return;
  settings_.setValue("MainWindow/lastDirectory",
                     QFileInfo(filename).absolutePath());
  addToRecentFiles(filename);
  this->openFile(filename);
}

void MainWindow::onFitScene() {
  app_->FitScene();
}

void MainWindow::loadRecentFiles() {
  recent_files_.clear();
  int size = settings_.beginReadArray("MainWindow/recentFiles");
  for (int i = 0; i < size; ++i) {
    settings_.setArrayIndex(i);
    QString file = settings_.value("file").toString();
    if (QFile::exists(file)) {
      recent_files_.append(file);
    }
  }
  settings_.endArray();
}

void MainWindow::saveRecentFiles() {
  settings_.beginWriteArray("MainWindow/recentFiles");
  for (int i = 0; i < recent_files_.size(); ++i) {
    settings_.setArrayIndex(i);
    settings_.setValue("file", recent_files_.at(i));
  }
  settings_.endArray();
}

void MainWindow::addToRecentFiles(const QString& filepath) {
  recent_files_.removeAll(filepath);
  recent_files_.prepend(filepath);
  while (recent_files_.size() > kMaxRecentFiles) {
    recent_files_.removeLast();
  }
  saveRecentFiles();
  updateRecentFilesMenu();
}

void MainWindow::updateRecentFilesMenu() {
  if (!recent_files_menu_) return;
  recent_files_menu_->clear();
  if (recent_files_.isEmpty()) {
    QAction* act = new QAction(tr("No recent files"), this);
    act->setEnabled(false);
    recent_files_menu_->addAction(act);
    return;
  }
  for (const QString& file : recent_files_) {
    QAction* act = new QAction(QFileInfo(file).fileName(), this);
    act->setData(file);
    act->setStatusTip(file);
    connect(act, SIGNAL(triggered()), this, SLOT(onOpenRecentFile()));
    recent_files_menu_->addAction(act);
  }
}

void MainWindow::onOpenRecentFile() {
  QAction* action = qobject_cast<QAction*>(sender());
  if (!action) return;
  QString filename = action->data().toString();
  if (!QFile::exists(filename)) {
    recent_files_.removeAll(filename);
    saveRecentFiles();
    updateRecentFilesMenu();
    return;
  }
  this->openFile(filename);
  addToRecentFiles(filename);
}
