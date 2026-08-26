#include "scenewindow.h"

#include "qevent.h"
#include "qvulkaninstance.h"

#include "numgeom/application.h"
#include "numgeom/scene.h"
#include "numgeom/userinputcontroller.h"
#include "numgeom/vkscenerenderer.h"
#include "numgeom/workbench.h"

SceneWindow::SceneWindow(Application* app) {
  this->setSurfaceType(QSurface::VulkanSurface);

  workbench_ = Workbench::Create("OCC");
  assert(workbench_ != nullptr);
  app->Add(workbench_);
}

bool SceneWindow::Initialize(QVulkanInstance* vulkan_instance) {
  if (!vulkan_instance->isValid())
    return false;
  this->setVulkanInstance(vulkan_instance);
  VkSurfaceKHR surface = QVulkanInstance::surfaceForWindow(this);
  assert(surface != VK_NULL_HANDLE);
  Scene* scene = workbench_->GetScene();
  scene->InitializeGraphics(
    [this]() {
      QSize sz = this->size();
      qreal r = this->devicePixelRatio();
      uint32_t width = static_cast<uint32_t>(sz.width() * r + 0.5);
      uint32_t height = static_cast<uint32_t>(sz.height() * r + 0.5);
      return std::make_tuple(width, height);
    },
    reinterpret_cast<uint64_t>(surface));
  return true;
}

SceneWindow::~SceneWindow() {
  delete workbench_;
}

void SceneWindow::keyPressEvent(QKeyEvent* event) {
  auto uic = workbench_->GetUserInputController();
  int key = event->key();
  uic->KeyPressed(key);
  QWindow::keyPressEvent(event);
}

void SceneWindow::keyReleaseEvent(QKeyEvent* event) {
  auto uic = workbench_->GetUserInputController();
  int key = event->key();
  uic->KeyReleased(key);
  QWindow::keyPressEvent(event);
}

void SceneWindow::mousePressEvent(QMouseEvent* event) {
  auto uic = workbench_->GetUserInputController();
  QPoint pt = event->pos();
  if (event->button() == Qt::LeftButton)
    uic->MouseLeftButtonDown(pt.x(), pt.y());
  else if (event->button() == Qt::RightButton)
    uic->MouseRightButtonDown(pt.x(), pt.y());
  else if (event->button() == Qt::MiddleButton)
    uic->MouseMiddleButtonDown(pt.x(), pt.y());
  else {
    event->ignore();
    return;
  }

  event->accept();
}

void SceneWindow::mouseReleaseEvent(QMouseEvent* event) {
  auto uic = workbench_->GetUserInputController();
  QPoint pt = event->pos();
  if (event->button() == Qt::LeftButton)
    uic->MouseLeftButtonUp(pt.x(), pt.y());
  else if (event->button() == Qt::RightButton)
    uic->MouseRightButtonUp(pt.x(), pt.y());
  else if (event->button() == Qt::MiddleButton)
    uic->MouseMiddleButtonUp(pt.x(), pt.y());
  else {
    event->ignore();
    return;
  }

  event->accept();
}

void SceneWindow::mouseMoveEvent(QMouseEvent* event) {
  auto uic = workbench_->GetUserInputController();
  QPoint pt = event->pos();
  uic->MouseMove(pt.x(), pt.y());
  event->accept();
}

void SceneWindow::wheelEvent(QWheelEvent* event) {
  auto uic = workbench_->GetUserInputController();
  QPoint angleDelta = event->angleDelta();
  int numDegrees = angleDelta.y() / 8;
  uic->MouseWheelRotate(numDegrees);
  event->accept();
}

namespace {
MouseButton ToMouseButton(Qt::MouseButton button) {
  switch (button) {
    case Qt::LeftButton:   return MouseButton::Left;
    case Qt::RightButton:  return MouseButton::Right;
    case Qt::MiddleButton: return MouseButton::Middle;
    default: assert(false);
  }
  std::abort();
}
}

void SceneWindow::mouseDoubleClickEvent(QMouseEvent* event) {
  auto uic = workbench_->GetUserInputController();
  QPoint pt = event->pos();
  uic->MouseDoubleClick(ToMouseButton(event->button()), pt.x(), pt.y());
  event->accept();
}

void SceneWindow::resizeEvent(QResizeEvent* event) {}

void SceneWindow::exposeEvent(QExposeEvent* event) {
  if (this->isExposed()) {
    workbench_->UpdateScreen();
  } else {
    // renderer_->finalize();
  }
}

bool SceneWindow::event(QEvent* e) {
  switch (e->type()) {
    case QEvent::Paint:
    case QEvent::UpdateRequest:
      workbench_->UpdateScreen();
      break;
    case QEvent::PlatformSurface: {
      auto* pse = static_cast<QPlatformSurfaceEvent*>(e);
      if (pse->surfaceEventType() ==
          QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed) {
        workbench_->Remove();
        workbench_->UpdateScreen();
        delete workbench_;
        workbench_ = nullptr;
      }
      break;
    }
  }
  return QWindow::event(e);
}

Workbench* SceneWindow::GetWorkbench() {
  return workbench_;
}
