#include "scenewindow.h"

#include <iostream>

#include "qevent.h"
#include "qvulkaninstance.h"

#include "numgeom/application.h"
#include "numgeom/scene.h"
#include "numgeom/userinputcontroller.h"
#include "numgeom/vkscenerenderer.h"

SceneWindow::SceneWindow(Application* app) {
  app_ = app;
}

bool SceneWindow::Initialize(QVulkanInstance* vulkan_instance,
                             const QString& scene_name) {
  this->setSurfaceType(QSurface::VulkanSurface);
  scene_ = app_->AddScene(scene_name.toStdString());
  user_input_controller_ = new UserInputController(scene_, app_->GetRenderer());

  // Устанавливаем функцию размера viewport'а (уже установлена в SceneMdiSubWindow,
  // но можно обновить, если нужно)
  scene_->SetViewportSizeFunction([this]() {
    QSize sz = this->size();
    qreal r = this->devicePixelRatio();
    uint32_t width = static_cast<uint32_t>(sz.width() * r);
    uint32_t height = static_cast<uint32_t>(sz.height() * r);
    return std::make_tuple(width, height);
  });

  if (!vulkan_instance->isValid())
    return false;

  this->setVulkanInstance(vulkan_instance);
  VkSurfaceKHR surface = QVulkanInstance::surfaceForWindow(this);
  assert(surface != VK_NULL_HANDLE);
  scene_->SetVulkanSurface(reinterpret_cast<uint64_t>(surface));

  return true;
}

SceneWindow::~SceneWindow() {
  delete user_input_controller_;
}

void SceneWindow::keyPressEvent(QKeyEvent* event) {
  int key = event->key();
  user_input_controller_->keyPressed(key);
  QWindow::keyPressEvent(event);
}

void SceneWindow::keyReleaseEvent(QKeyEvent* event) {
  int key = event->key();
  user_input_controller_->keyReleased(key);
  QWindow::keyPressEvent(event);
}

void SceneWindow::mousePressEvent(QMouseEvent* event) {
  QPoint pt = event->pos();
  if (event->button() == Qt::LeftButton)
    user_input_controller_->mouseLeftButtonDown(pt.x(), pt.y());
  else if (event->button() == Qt::RightButton)
    user_input_controller_->mouseRightButtonDown(pt.x(), pt.y());
  else if (event->button() == Qt::MiddleButton)
    user_input_controller_->mouseMiddleButtonDown(pt.x(), pt.y());
  else {
    event->ignore();
    return;
  }

  event->accept();
}

void SceneWindow::mouseReleaseEvent(QMouseEvent* event) {
  QPoint pt = event->pos();
  if (event->button() == Qt::LeftButton)
    user_input_controller_->mouseLeftButtonUp(pt.x(), pt.y());
  else if (event->button() == Qt::RightButton)
    user_input_controller_->mouseRightButtonUp(pt.x(), pt.y());
  else if (event->button() == Qt::MiddleButton)
    user_input_controller_->mouseMiddleButtonUp(pt.x(), pt.y());
  else {
    event->ignore();
    return;
  }

  event->accept();
}

void SceneWindow::mouseMoveEvent(QMouseEvent* event) {
  QPoint pt = event->pos();
  user_input_controller_->mouseMove(pt.x(), pt.y());
  event->accept();
}

void SceneWindow::wheelEvent(QWheelEvent* event) {
  QPoint angleDelta = event->angleDelta();
  int numDegrees = angleDelta.y() / 8;
  user_input_controller_->mouseWheelRotate(numDegrees);
  event->accept();
}

void SceneWindow::resizeEvent(QResizeEvent* event) {}

void SceneWindow::exposeEvent(QExposeEvent* event) {
  if (this->isExposed()) {
    app_->GetRenderer()->Update(scene_);
  } else {
    // renderer_->finalize();
  }
}

bool SceneWindow::event(QEvent* e) {
  switch (e->type()) {
    case QEvent::Paint:
    case QEvent::UpdateRequest:
      app_->GetRenderer()->Update(scene_);
      break;
    case QEvent::PlatformSurface: {
      auto* pse = static_cast<QPlatformSurfaceEvent*>(e);
      if (pse->surfaceEventType() ==
          QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed) {
        app_->RemoveScene(scene_);
        app_->GetRenderer()->Update(scene_);
        scene_ = nullptr;
      }
      break;
    }
  }
  return QWindow::event(e);
}
