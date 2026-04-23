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

 public slots:
  void onScreenshot();
  void onOpenFile();
  void onQuit();
  void onFitScene();

 private:
  Application* app_;
  SceneWindow* scene_window_;
  QVulkanInstance vulkan_instance_;
  QSettings settings_;
};
#endif  // NUMGEOM_EXAMPLE_APPQT_MAINWINDOW_H_
