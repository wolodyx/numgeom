#ifndef NUMGEOM_EXAMPLE_APPQT_MAINWINDOW_H_
#define NUMGEOM_EXAMPLE_APPQT_MAINWINDOW_H_

#include "qmainwindow.h"
#include "qsettings.h"
#include "qvulkaninstance.h"

class Application;
class SceneWindow;

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  MainWindow(Application* app);
  ~MainWindow();
  void initVulkan();

 private:
  void createActions();
  void updateRecentFilesMenu();
  void loadRecentFiles();
  void saveRecentFiles();
  void addToRecentFiles(const QString& filepath);
  void openFile(const QString&);

 private slots:
  void onScreenshot();
  void onOpenFile();
  void onOpenRecentFile();
  void onQuit();
  void onFitScene();

 private:
  Application* app_;
  SceneWindow* scene_window_;
  QVulkanInstance vulkan_instance_;
  QSettings settings_;
  QStringList recent_files_;
  static const int kMaxRecentFiles = 10;
  QMenu* recent_files_menu_;
};
#endif  // NUMGEOM_EXAMPLE_APPQT_MAINWINDOW_H_
