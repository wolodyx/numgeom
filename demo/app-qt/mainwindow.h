#ifndef NUMGEOM_EXAMPLE_APPQT_MAINWINDOW_H_
#define NUMGEOM_EXAMPLE_APPQT_MAINWINDOW_H_

#include "qmainwindow.h"
#include "qmdiarea.h"
#include "qsettings.h"
#include "qvulkaninstance.h"

class Application;
class Scene;
class SceneWindow;
class SceneMdiSubWindow;

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  MainWindow(Application* app);
  ~MainWindow();
  void initVulkan();

  // Возвращает активное MDI подокно (или nullptr, если нет окон)
  SceneMdiSubWindow* GetActiveMdiSubWindow() const;
  Scene* GetActiveScene() const;

 private:
  void addToRecentFiles(const QString& filepath);
  void createActions();
  void createFileMenu();
  void createHelpMenu();
  void createSceneMenu();
  void createViewMenu();
  void createWindowMenu();
  void loadRecentFiles();
  void openFile(const QString&);
  bool loadWelcomeModel(Scene*);
  void saveRecentFiles();
  void updateRecentFilesMenu();

  // Создает новое MDI подокно с заданным именем сцены
  SceneMdiSubWindow* CreateMdiSubWindow(const QString& scene_name);

 private slots:
  void onAbout();
  void onAddAxisIndicator();
  void onAddFgImage();
  void onAddFgText();
  void onDownloadData();
  void onFitScene();
  void onGoToTestData();
  void onMsaa(QAction* action);
  void onOpenFile();
  void onOpenRecentFile();
  void onQuit();
  void onScreenshot();
  void updateWindowMenu(); //!< Обновляет список окон в меню Window.

  // Window menu slots
  void onNewWindow();
  void onCloseWindow();
  void onCloseAllWindows();
  void onTileWindows();
  void onCascadeWindows();
  void onNextWindow();
  void onPreviousWindow();
  void onWindowActivated();

 private:
  Application* app_;
  QMdiArea* mdi_area_;
  QVulkanInstance vulkan_instance_;
  QSettings settings_;
  QStringList recent_files_;
  static const int kMaxRecentFiles = 10;
  QMenu* recent_files_menu_;
  QMenu* window_list_menu_;
  bool updating_window_menu_;
};
#endif  // NUMGEOM_EXAMPLE_APPQT_MAINWINDOW_H_
