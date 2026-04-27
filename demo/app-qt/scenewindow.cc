#include "scenewindow.h"

#include <iostream>

#include "numgeom/application.h"
#include "numgeom/gpumanager.h"
#include "numgeom/userinputcontroller.h"
#include "qevent.h"
#include "qvulkaninstance.h"

SceneWindow::SceneWindow(Application* app) {
  gpu_manager_ = app->GetGpuManager();
  user_input_controller_ = new UserInputController(app);
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
    gpu_manager_->update();
  } else {
    // gpu_manager_->finalize();
  }
}

bool SceneWindow::event(QEvent* e) {
  switch (e->type()) {
    case QEvent::Paint:
    case QEvent::UpdateRequest:
      gpu_manager_->update();
      break;
    case QEvent::PlatformSurface: {
      auto* pse = static_cast<QPlatformSurfaceEvent*>(e);
      if (pse->surfaceEventType() ==
          QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed) {
        gpu_manager_->finalize();
      }
      break;
    }
  }
  return QWindow::event(e);
}
