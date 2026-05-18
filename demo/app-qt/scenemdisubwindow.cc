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
    : QMdiSubWindow(parent),
      app_(app),
      scene_name_(scene_name) {
  scene_window_ = new SceneWindow(app_);

  QWidget* container = QWidget::createWindowContainer(scene_window_, this);
  setWidget(container);

  // Set window title
  setWindowTitle(scene_name_);
}

SceneMdiSubWindow::~SceneMdiSubWindow() {
  scene_window_->destroy();
  //delete scene_window_;
}

SceneWindow* SceneMdiSubWindow::GetSceneWindow() const {
  return scene_window_;
}

Scene* SceneMdiSubWindow::GetScene() const {
  return app_->GetScene(scene_name_.toStdString());
}

QString SceneMdiSubWindow::GetSceneName() const {
  return scene_name_;
}
