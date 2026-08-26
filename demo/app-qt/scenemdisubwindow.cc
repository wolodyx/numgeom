#include "scenemdisubwindow.h"

#include <iostream>

#include <QCloseEvent>
#include <QWidget>
#include <QSurface>
#include "qvulkaninstance.h"

#include "numgeom/application.h"
#include "numgeom/vkscenerenderer.h"
#include "numgeom/scene.h"
#include "scenewindow.h"

SceneMdiSubWindow::SceneMdiSubWindow(Application* app,
                                     const QString& scene_name,
                                     QVulkanInstance* vulkan_instance,
                                     QWidget* parent)
    : QMdiSubWindow(parent), app_(app) {
  scene_window_ = new SceneWindow(app_);

  QWidget* container = QWidget::createWindowContainer(scene_window_, this);
  setWidget(container);

  setWindowTitle(scene_name);
}

SceneMdiSubWindow::~SceneMdiSubWindow() {
  scene_window_->destroy();
  //delete scene_window_;
}

SceneWindow* SceneMdiSubWindow::GetSceneWindow() const {
  return scene_window_;
}

Workbench* SceneMdiSubWindow::GetWorkbench() const {
  return scene_window_->GetWorkbench();
}
