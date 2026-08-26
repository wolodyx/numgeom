#ifndef NUMGEOM_EXAMPLES_APPQT_SCENEMDISUBWINDOW_H_
#define NUMGEOM_EXAMPLES_APPQT_SCENEMDISUBWINDOW_H_

#include "qmdisubwindow.h"

class Application;
class QVulkanInstance;
class SceneWindow;
class Workbench;

/** \class SceneMdiSubWindow
\brief MDI подокно, содержащее SceneWindow и управляющее своей сценой.
*/
class SceneMdiSubWindow : public QMdiSubWindow {
  Q_OBJECT

 public:
  // Создает подокно с заданным именем сцены и указателем на приложение.
  // Имя сцены используется для создания уникальной сцены в Application.
  SceneMdiSubWindow(Application* app, const QString& scene_name,
                    QVulkanInstance* vk_instance, QWidget* parent = nullptr);
  ~SceneMdiSubWindow();

  // Возвращает указатель на вложенное SceneWindow.
  SceneWindow* GetSceneWindow() const;

  Workbench* GetWorkbench() const;

 private:
  Application* app_;
  SceneWindow* scene_window_;
  QString scene_name_;
};

#endif  // NUMGEOM_EXAMPLES_APPQT_SCENEMDISUBWINDOW_H_
