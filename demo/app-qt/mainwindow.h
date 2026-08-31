#ifndef NUMGEOM_EXAMPLE_APPQT_MAINWINDOW_H_
#define NUMGEOM_EXAMPLE_APPQT_MAINWINDOW_H_

#include "qmainwindow.h"
#include "qmdiarea.h"
#include "qsettings.h"
#include "qvulkaninstance.h"

class Application;
class GuiMediator;
class Scene;
class SceneWindow;
class SceneMdiSubWindow;
class Workbench;

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  MainWindow(Application* app);
  ~MainWindow();
  void initVulkan();

  // Возвращает активное MDI подокно (или nullptr, если нет окон)
  SceneMdiSubWindow* GetActiveMdiSubWindow() const;
  Workbench* GetActiveWorkbench() const;

  void onCommandMode();
  void offCommandMode();

 private:
  void addToRecentFiles(const QString& filepath);
  void createActions();
  void createFileMenu();
  void createHelpMenu();
  void createPickingMenu();
  void createSceneMenu();
  void createViewMenu();
  void createWindowMenu();
  void createWorkbenchMenus();
  void executeCommand(const std::string&);
  void loadRecentFiles();
  void loadWelcomeModel(Scene*);
  void openFile(const QString&);
  void saveRecentFiles();
  void updateRecentFilesMenu();
  SceneMdiSubWindow* CreateMdiSubWindow(const QString& scene_name);

 private slots:
  void onAbout();
  void onAddArrow();
  void onAddAxisIndicator();
  void onAddFgImage();
  void onAddFgText();
  void onClearScene();
  void onDownloadData();
  void onFitScene();
  void onGoToTestData();
  void onMsaa(QAction* action);
  void onOpenFile();
  void onOpenRecentFile();
  void onPickingMode(QAction* action);
  void onQuit();
  void onRedrawScene();
  void onScreenshot();
  void updateWindowMenu();

  //! \brief Window menu slots
  //! \{
  void onNewWindow();
  void onCloseWindow();
  void onCloseAllWindows();
  void onTileWindows();
  void onCascadeWindows();
  void onNextWindow();
  void onPreviousWindow();
  void onWindowActivated();
  //! \}

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
  QDockWidget* command_args_widget_;
  GuiMediator* gui_mediator_;
};
#endif  // NUMGEOM_EXAMPLE_APPQT_MAINWINDOW_H_
