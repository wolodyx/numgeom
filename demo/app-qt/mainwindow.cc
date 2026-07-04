#include "mainwindow.h"

#include <iostream>

#include "qactiongroup.h"
#include "qapplication.h"
#include "qdesktopservices.h"
#include "qdir.h"
#include "qevent.h"
#include "qfile.h"
#include "qfiledialog.h"
#include "qfileinfo.h"
#include "qmenu.h"
#include "qmenubar.h"
#include "qmessagebox.h"
#include "qresource.h"
#include "qscreen.h"
#include "qstandardpaths.h"

#include "numgeom/application.h"
#include "numgeom/fgtext.h"
#include "numgeom/scene.h"
#include "numgeom/sceneobject_mesh.h"
#include "numgeom/scenewidget_axisindicator.h"
#include "numgeom/vkscenerenderer.h"
#ifdef USE_NUMGEOM_MODULE_OCC
#  include "numgeom/loadusingocc.h"
#  include "numgeom/sceneobject_polytriangulation.h"
#  include "numgeom/sceneobject_tdocstd_document.h"
#endif

#include "dialogs/addforegroundimage.h"
#include "dialogs/addforegroundtext.h"

#include "downloaddata.h"
#include "loadtotrimesh.h"
#include "scenewindow.h"
#include "scenemdisubwindow.h"
#include "version.h"

MainWindow::MainWindow(Application* app) : settings_("NumGeom", "QtDemo"), updating_window_menu_(false) {
  app_ = app;
  loadRecentFiles();
  mdi_area_ = new QMdiArea(this);
  this->setCentralWidget(mdi_area_);
  this->createActions();
  QIcon window_icon(QString(":/resources/icons/app.ico"));
  this->setWindowIcon(window_icon);
}

MainWindow::~MainWindow() {}

SceneMdiSubWindow* MainWindow::GetActiveMdiSubWindow() const {
  QMdiSubWindow* sub = mdi_area_->activeSubWindow();
  return qobject_cast<SceneMdiSubWindow*>(sub);
}

Scene* MainWindow::GetActiveScene() const {
  auto sub_window = GetActiveMdiSubWindow();
  if (!sub_window)
    return nullptr;
  auto scene_sub_window = qobject_cast<SceneMdiSubWindow*>(sub_window);
  if (!scene_sub_window)
    return nullptr;
  return scene_sub_window->GetScene();
}

SceneMdiSubWindow* MainWindow::CreateMdiSubWindow(const QString& scene_name) {
  SceneMdiSubWindow* sub = new SceneMdiSubWindow(app_, scene_name, &vulkan_instance_, mdi_area_);
  sub->setAttribute(Qt::WA_DeleteOnClose);
  mdi_area_->addSubWindow(sub);
  sub->showMaximized();
  connect(sub, SIGNAL(destroyed(QObject*)), this, SLOT(updateWindowMenu()));
  updateWindowMenu();
  SceneWindow* scene_window = sub->GetSceneWindow();
  scene_window->Initialize(&vulkan_instance_, scene_name);
  mdi_area_->setActiveSubWindow(sub);
  return sub;
}

void MainWindow::initVulkan() {
  auto* renderer = app_->GetRenderer();
  vulkan_instance_.setVkInstance(renderer->GetInstance());
  if (!vulkan_instance_.create()) qFatal("Vulkan instance creating error");

  CreateMdiSubWindow("mainscene");

  // Получаем активное MDI подокно (первое созданное)
  SceneMdiSubWindow* active_sub = GetActiveMdiSubWindow();
  if (!active_sub) {
    qFatal("No active MDI subwindow found");
    return;
  }

  // Сцена уже создана в конструкторе SceneMdiSubWindow
  Scene* scene = active_sub->GetScene();
  if (!scene) {
    qFatal("Failed to get scene from active subwindow");
    return;
  }

  this->loadWelcomeModel(scene);

  //auto fg_scene = app_->AddScene("axis-indicator", scene);
  //fg_scene->AddObject<SceneWidget_AxisIndicator>();
  //fg_scene->SetViewportSizeFunction([this]() {
  //  return std::make_tuple(100, 100);
  //});

  // Загружаем логотип
  QFile resource_file(":/resources/logo.png");
  if (!resource_file.open(QIODevice::ReadOnly)) {
    // Log error but don't crash - application can work without logo
    qWarning() << "WARNING: Could not open logo resource file";
  } else {
    QByteArray resource_data = resource_file.readAll();
    resource_file.close();
    FgObject* fg_image = app_->AddFgImage(
        reinterpret_cast<const unsigned char*>(resource_data.constData()),
        resource_data.size());
    scene->AddFgObject(fg_image, glm::ivec2(5,5));
  }

  FgText* fg_text = app_->AddFgText("Text rendering test");
  scene->AddFgObject(fg_text, glm::ivec2(5,100));

  scene->FitScene();
}

bool MainWindow::loadWelcomeModel(Scene* scene) {
  QString data_path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  if (data_path.isEmpty())
    return false;

  QFile resource_file(":/resources/welcome-model.vtk");
  if (!resource_file.open(QIODevice::ReadOnly))
    return false;
  QByteArray resource_data = resource_file.readAll();
  resource_file.close();

  QDir dir;
  if (!dir.mkpath(data_path))
    return false;

  QString model_filename = data_path + "/welcome-model.vtk";
  QFile model_file(model_filename);
  if (!model_file.open(QIODevice::WriteOnly))
    return false;
  model_file.write(resource_data);
  model_file.close();

  auto trimesh = LoadToTriMesh(model_filename.toStdString());
  scene->AddObject<SceneObject_Mesh>(trimesh);
  return true;
}

void MainWindow::createActions() {
  this->createFileMenu();
  this->createViewMenu();
  this->createSceneMenu();
  this->createWindowMenu();
  this->createHelpMenu();
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
            [=, this](){
              auto ortho_basis = OrthoBasis<float>::FromZY(
                  glm::vec3(0,1,0),
                  glm::vec3(0,0,1));
              auto scene = this->GetActiveScene();
              scene->OrientCamera(ortho_basis);
              app_->Update(scene);
    });
    act->setShortcut(QKeySequence(tr("1")));
    std_view->addAction(act);
  }
  {
    QAction* act = new QAction(tr("Top"), this);
    connect(act, &QAction::triggered,
            [=, this](){
              auto ortho_basis = OrthoBasis<float>::FromZY(
                  glm::vec3(0,0,-1),
                  glm::vec3(0,1,0));
              auto scene = this->GetActiveScene();
              scene->OrientCamera(ortho_basis);
              app_->Update(scene);
    });
    act->setShortcut(QKeySequence(tr("2")));
    std_view->addAction(act);
  }
  {
    QAction* act = new QAction(tr("Right"), this);
    connect(act, &QAction::triggered,
            [=, this](){
              auto ortho_basis = OrthoBasis<float>::FromZY(
                  glm::vec3(-1,0,0),
                  glm::vec3(0,0,1));
              auto scene = this->GetActiveScene();
              scene->OrientCamera(ortho_basis);
              app_->Update(scene);
    });
    act->setShortcut(QKeySequence(tr("3")));
    std_view->addAction(act);
  }
  {
    QAction* act = new QAction(tr("Rear"), this);
    connect(act, &QAction::triggered,
            [=, this](){
              auto ortho_basis = OrthoBasis<float>::FromZY(
                  glm::vec3(0,-1,0),
                  glm::vec3(0,0,1));
              auto scene = this->GetActiveScene();
              scene->OrientCamera(ortho_basis);
              app_->Update(scene);
    });
    act->setShortcut(QKeySequence(tr("4")));
    std_view->addAction(act);
  }
  {
    QAction* act = new QAction(tr("Bottom"), this);
    connect(act, &QAction::triggered,
            [=, this](){
              auto ortho_basis = OrthoBasis<float>::FromZY(
                  glm::vec3(0,0,1),
                  glm::vec3(0,-1,0));
              auto scene = this->GetActiveScene();
              scene->OrientCamera(ortho_basis);
              app_->Update(scene);
    });
    act->setShortcut(QKeySequence(tr("5")));
    std_view->addAction(act);
  }
  {
    QAction* act = new QAction(tr("Left"), this);
    connect(act, &QAction::triggered,
            [=, this](){
              auto ortho_basis = OrthoBasis<float>::FromZY(
                  glm::vec3(1,0,0),
                  glm::vec3(0,0,1));
              auto scene = this->GetActiveScene();
              scene->OrientCamera(ortho_basis);
              app_->Update(scene);
    });
    act->setShortcut(QKeySequence(tr("6")));
    std_view->addAction(act);
  }
}

void MainWindow::createSceneMenu() {
  QMenu* menu = menuBar()->addMenu(tr("&Scene"));

  {
    QIcon icon;
    icon.addFile(QString::fromUtf8(":/resources/icons/axis-indicator-16.png"),
                 QSize(), QIcon::Normal, QIcon::Off);
    QAction* act = new QAction(icon, tr("Add axis indicator"), this);
    connect(act, SIGNAL(triggered()), this, SLOT(onAddAxisIndicator()));
    menu->addAction(act);
  }
  {
    QIcon icon;
    icon.addFile(QString::fromUtf8(":/resources/icons/add-foreground-image-16.png"),
                 QSize(), QIcon::Normal, QIcon::Off);
    QAction* act = new QAction(icon, tr("Add foreground image"), this);
    connect(act, SIGNAL(triggered()), this, SLOT(onAddFgImage()));
    menu->addAction(act);
  }
  {
    QIcon icon;
    icon.addFile(QString::fromUtf8(":/resources/icons/add-foreground-text-16.png"),
                 QSize(), QIcon::Normal, QIcon::Off);
    QAction* act = new QAction(icon, tr("Add foreground text"), this);
    connect(act, SIGNAL(triggered()), this, SLOT(onAddFgText()));
    menu->addAction(act);
  }
}

void MainWindow::createWindowMenu() {
  QMenu* menu = menuBar()->addMenu(tr("&Window"));
  QActionGroup* window_action_group_ = new QActionGroup(this);
  window_action_group_->setExclusive(true);

  // New Window
  QAction* new_window_act = new QAction(tr("&New Window"), this);
  new_window_act->setShortcuts(QKeySequence::New);
  new_window_act->setStatusTip(tr("Create a new window"));
  connect(new_window_act, SIGNAL(triggered()), this, SLOT(onNewWindow()));
  menu->addAction(new_window_act);

  // Close Window
  QAction* close_window_act = new QAction(tr("&Close Window"), this);
  close_window_act->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_F4));
  close_window_act->setStatusTip(tr("Close the active window"));
  connect(close_window_act, SIGNAL(triggered()), this, SLOT(onCloseWindow()));
  menu->addAction(close_window_act);

  // Close All Windows
  QAction* close_all_act = new QAction(tr("Close All Windows"), this);
  close_all_act->setStatusTip(tr("Close all windows"));
  connect(close_all_act, SIGNAL(triggered()), this, SLOT(onCloseAllWindows()));
  menu->addAction(close_all_act);

  menu->addSeparator();

  // Tile Windows
  QAction* tile_act = new QAction(tr("&Tile"), this);
  tile_act->setStatusTip(tr("Tile the windows"));
  connect(tile_act, SIGNAL(triggered()), this, SLOT(onTileWindows()));
  menu->addAction(tile_act);

  // Cascade Windows
  QAction* cascade_act = new QAction(tr("&Cascade"), this);
  cascade_act->setStatusTip(tr("Cascade the windows"));
  connect(cascade_act, SIGNAL(triggered()), this, SLOT(onCascadeWindows()));
  menu->addAction(cascade_act);

  menu->addSeparator();

  // Next Window
  QAction* next_act = new QAction(tr("&Next"), this);
  next_act->setShortcut(QKeySequence(tr("Ctrl+Tab")));
  next_act->setStatusTip(tr("Move focus to next window"));
  connect(next_act, SIGNAL(triggered()), this, SLOT(onNextWindow()));
  menu->addAction(next_act);

  // Previous Window
  QAction* prev_act = new QAction(tr("&Previous"), this);
  prev_act->setShortcut(QKeySequence(tr("Ctrl+Shift+Tab")));
  prev_act->setStatusTip(tr("Move focus to previous window"));
  connect(prev_act, SIGNAL(triggered()), this, SLOT(onPreviousWindow()));
  menu->addAction(prev_act);

  menu->addSeparator();

  // Dynamic window list submenu
  window_list_menu_ = menu->addMenu(tr("Windows"));
  updateWindowMenu();

  connect(mdi_area_, SIGNAL(subWindowActivated(QMdiSubWindow*)),
          this, SLOT(updateWindowMenu()));
}

void MainWindow::createHelpMenu() {
  QMenu* menu = menuBar()->addMenu(tr("&Help"));

  auto test_data_menu = menu->addMenu(tr("Test data"));
  { // Download data
    QIcon icon;
    icon.addFile(QString::fromUtf8(":/resources/icons/download-data.ico"),
                 QSize(), QIcon::Normal, QIcon::Off);
    QAction* act = new QAction(icon, tr("Download data"), this);
    act->setStatusTip(tr("Download an archive via HTTP"));
    connect(act, SIGNAL(triggered()), this, SLOT(onDownloadData()));
    test_data_menu->addAction(act);
  }
  { // Go to test data directory
    QIcon icon;
    icon.addFile(QString::fromUtf8(":/resources/icons/goto-testdata.ico"),
                 QSize(), QIcon::Normal, QIcon::Off);
    QAction* act = new QAction(icon, tr("Go to test dir"), this);
    act->setStatusTip(tr("Open test data directory"));
    connect(act, SIGNAL(triggered()), this, SLOT(onGoToTestData()));
    test_data_menu->addAction(act);
  }

  menu->addSeparator();

  { // About
    QIcon icon;
    icon.addFile(QString::fromUtf8(":/resources/icons/about-16.png"),
                 QSize(), QIcon::Normal, QIcon::Off);
    QAction* act = new QAction(icon, tr("About"), this);
    act->setStatusTip(tr("About application"));
    connect(act, SIGNAL(triggered()), this, SLOT(onAbout()));
    menu->addAction(act);
  }
}

void MainWindow::onDownloadData() {
  const std::string url = "https://storage.yandexcloud.net/numgeom-storage/numgeom-testdata.zip";
  const QString output_dir = QDir::homePath();
  bool res = DownloadData(url, output_dir.toStdString());
  if (!res) {
    QMessageBox::warning(this, tr("Download failed"),
                         tr("Failed to download archive."));
    return;
  }
  const QString testdata_dir = output_dir + "/numgeom-testdata";
  settings_.setValue("MainWindow/testDataDirectory", testdata_dir);
  settings_.setValue("MainWindow/lastDirectory", testdata_dir);
  settings_.setValue("MainWindow/lastFgImageDirectory", testdata_dir);
  QMessageBox::information(this, tr("Download complete"),
                           tr("Archive downloaded successfully to:\n%1")
                               .arg(output_dir),
                           QMessageBox::Ok);
}

void MainWindow::onGoToTestData() {
  QString testdata_dir =
      settings_.value("MainWindow/testDataDirectory")
               .toString();
  if (testdata_dir.isEmpty()) {
    qDebug() << "The directory with the test data is not specified.";
    return;
  }
  QDesktopServices::openUrl(QUrl::fromLocalFile(testdata_dir));
}

void MainWindow::onAbout() {
  QString version = QString::fromUtf8(NUMGEOM_APPQT_VERSION);
  QString text = tr("Demo app for NumGeom framework") + "\n";
  text += tr("Version: ") + version;
  QMessageBox message_box;
  message_box.setWindowTitle(tr("About demo app"));
  message_box.setText(text);
  message_box.setIcon(QMessageBox::Information);
  message_box.setStandardButtons(QMessageBox::Close);
  message_box.exec();
}

void MainWindow::onScreenshot() {
  SceneMdiSubWindow* active_sub = GetActiveMdiSubWindow();
  if (!active_sub) {
    qWarning() << "No active window for screenshot";
    return;
  }

  QString filename = "screen.png";
  QScreen* screen = QApplication::primaryScreen();

  // Get the global position and size of the active subwindow
  QPoint global_pos = active_sub->mapToGlobal(QPoint(0, 0));
  QSize size = active_sub->size();

  QPixmap screenshot = screen->grabWindow(
      0, global_pos.x(), global_pos.y(), size.width(), size.height());
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
  SceneMdiSubWindow* active_sub = GetActiveMdiSubWindow();
  if (!active_sub) return;
  Scene* scene = active_sub->GetScene();
  if (!scene) return;

  // Update window title with filename
  QFileInfo fileInfo(filename);
  active_sub->setWindowTitle(fileInfo.fileName());

  scene->Clear();
  app_->Update(scene);
#ifdef USE_NUMGEOM_MODULE_OCC
  if (IsStepFile(filename)) {
    auto document = LoadStepDocument(filename.toStdWString());
    if (!document) {
      qDebug() << "Ошибка при загрузке файла '" << filename << "'";
      return;
    }
    scene->AddObject<SceneObject_TDocStd_Document>(document);
    scene->FitScene();
    app_->Update(scene);
    return;
  } else if (IsStlFile(filename)) {
    auto triangulation = LoadStl(filename.toStdWString());
    if (!triangulation) {
      qDebug() << "Ошибка при загрузке файла `" << filename << "'";
      return;
    }
    scene->AddObject<SceneObject_PolyTriangulation>(triangulation);
    scene->FitScene();
    app_->Update(scene);
    return;
  } else if (IsIgesFile(filename)) {
    auto document = LoadIges(filename.toStdWString());
    if (!document) {
      qDebug() << "Ошибка при загрузке файла `" << filename << "'";
      return;
    }
    scene->AddObject<SceneObject_TDocStd_Document>(document);
    scene->FitScene();
    app_->Update(scene);
    return;
  } else if (IsObjFile(filename)) {
    auto triangulation = LoadObj(filename.toStdWString());
    if (!triangulation) {
      qDebug() << "Ошибка при загрузке файла `" << filename << "'";
      return;
    }
    scene->AddObject<SceneObject_PolyTriangulation>(triangulation);
    scene->FitScene();
    app_->Update(scene);
    return;
#ifdef NUMGEOM_OCC_LOAD_VRML
  } else if (IsVrmlFile(filename)) {
    auto document = LoadVrml(filename.toStdWString());
    if (!document) {
      qDebug() << "Ошибка при загрузке файла `" << filename << "'";
      return;
    }
    scene->AddObject<SceneObject_TDocStd_Document>(document);
    scene->FitScene();
    app_->Update(scene);
    return;
#endif
  } else if (IsGltfFile(filename)) {
    auto document = LoadGltf(filename.toStdWString());
    if (!document) {
      qDebug() << "Ошибка при загрузке файла `" << filename << "'";
      return;
    }
    scene->AddObject<SceneObject_TDocStd_Document>(document);
    scene->FitScene();
    app_->Update(scene);
    return;
  }
#endif
  auto mesh = LoadToTriMesh(filename.toStdWString());
  if(mesh) {
    scene->AddObject<SceneObject_Mesh>(mesh);
    scene->FitScene();
    app_->Update(scene);
  }
}

void MainWindow::onOpenFile() {
  QString last_directory =
    settings_.value("MainWindow/lastDirectory", QDir::homePath()).toString();
  SceneMdiSubWindow* active_sub = GetActiveMdiSubWindow();
  QString filename = QFileDialog::getOpenFileName(this, tr("Select open file"),
                                                  last_directory);
  if (filename.isEmpty()) return;
  // NOTE: After a modal dialog (QFileDialog), QMdiArea may lose track of the
  // active subwindow because the native dialog steals focus and triggers
  // subWindowActivated(nullptr) during the nested event loop.
  // Re-activate the previously known subwindow if needed.
  if (!GetActiveMdiSubWindow() && active_sub) {
    mdi_area_->setActiveSubWindow(active_sub);
  }
  settings_.setValue("MainWindow/lastDirectory",
                     QFileInfo(filename).absolutePath());
  addToRecentFiles(filename);
  this->openFile(filename);
}

void MainWindow::onFitScene() {
  SceneMdiSubWindow* active_sub = GetActiveMdiSubWindow();
  if (!active_sub) return;
  Scene* scene = active_sub->GetScene();
  if (!scene) return;
  scene->FitScene();
  app_->Update(scene);
}

void MainWindow::onAddAxisIndicator() {
  SceneMdiSubWindow* active_sub = GetActiveMdiSubWindow();
  if (!active_sub) return;
  Scene* scene = active_sub->GetScene();
  if (!scene) return;
  scene->Clear();
  scene->AddObject<SceneWidget_AxisIndicator>();
  scene->FitScene();
  app_->Update(scene);
}

void MainWindow::onAddFgImage() {
  SceneMdiSubWindow* active_sub = GetActiveMdiSubWindow();
  if (!active_sub)
    return;
  Scene* scene = active_sub->GetScene();
  assert(scene != nullptr);

  Dialog_AddForegroundImage dialog(this, settings_);
  if (dialog.exec() != QDialog::Accepted)
    return;

  const std::string file_path = dialog.GetFilePath();
  if (file_path.empty())
    return;

  FgObject* fg = app_->AddFgImage(file_path);
  scene->AddFgObject(fg, dialog.GetPosition());
  app_->Update(scene);
}

void MainWindow::onAddFgText() {
  SceneMdiSubWindow* active_sub = GetActiveMdiSubWindow();
  if (!active_sub)
    return;
  Scene* scene = active_sub->GetScene();
  assert(scene != nullptr);

  Dialog_AddForegroundText dialog(this);
  if (dialog.exec() != QDialog::Accepted)
    return;

  FgText* fg = app_->AddFgText(dialog.GetText());
  fg->SetColor(dialog.GetColor());
  scene->AddFgObject(fg, dialog.GetPosition());
  app_->Update(scene);
}

void MainWindow::onNewWindow() {
  static int window_count = 0;
  CreateMdiSubWindow(QString("scene_%1").arg(++window_count));
}

void MainWindow::onCloseWindow() {
  QMdiSubWindow* active = mdi_area_->activeSubWindow();
  if (active) {
    active->close();
    // Menu will be updated via destroyed signal, but update immediately
    updateWindowMenu();
  }
}

void MainWindow::onCloseAllWindows() {
  mdi_area_->closeAllSubWindows();
  // Menu will be updated via destroyed signals, but update immediately
  updateWindowMenu();
  // If all windows were closed, create a new one
  if (mdi_area_->subWindowList().isEmpty()) {
    CreateMdiSubWindow(tr("Scene %1").arg(1));
  }
}

void MainWindow::onTileWindows() {
  mdi_area_->tileSubWindows();
}

void MainWindow::onCascadeWindows() {
  mdi_area_->cascadeSubWindows();
}

void MainWindow::onNextWindow() {
  mdi_area_->activateNextSubWindow();
}

void MainWindow::onPreviousWindow() {
  mdi_area_->activatePreviousSubWindow();
}

void MainWindow::onWindowActivated() {
  QAction* s = dynamic_cast<QAction*>(sender());
  if (!s || !s->data().isValid()) return;
  int index = s->data().toInt();
  QList<QMdiSubWindow*> windows = mdi_area_->subWindowList();
  if (index >= 0 && index < windows.size()) {
    mdi_area_->setActiveSubWindow(windows.at(index));
  }
}

void MainWindow::updateWindowMenu() {
  // Guard against recursive calls
  if (updating_window_menu_) {
    return;
  }
  updating_window_menu_ = true;

  // Safety check: ensure window_list_menu_ is initialized
  if (!window_list_menu_) {
    updating_window_menu_ = false;
    return;
  }

  window_list_menu_->clear();
  QList<QMdiSubWindow*> windows = mdi_area_->subWindowList();
  if (windows.isEmpty()) {
    QAction* act = new QAction(tr("No windows"), this);
    act->setEnabled(false);
    window_list_menu_->addAction(act);
    updating_window_menu_ = false;
    return;
  }
  QMdiSubWindow* active = mdi_area_->activeSubWindow();
  for (int i = 0; i < windows.size(); ++i) {
    QMdiSubWindow* win = windows.at(i);
    QString title = win->windowTitle();
    if (title.isEmpty()) {
      title = tr("Window %1").arg(i + 1);
    }
    QAction* act = new QAction(title, this);
    act->setCheckable(true);
    act->setChecked(win == active);
    act->setData(i);
    connect(act, SIGNAL(triggered()), this, SLOT(onWindowActivated()));
    window_list_menu_->addAction(act);
  }

  updating_window_menu_ = false;
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
