#include "numgeom/userinputcontroller.h"

#include "numgeom/scene.h"
#include "numgeom/vkscenerenderer.h"

struct MouseButtonState {
  bool down = false;
  int xDown = 0, yDown = 0;
  int xPrev = 0, yPrev = 0;
  glm::vec3 clickedPoint;
  glm::vec3 prevDraggedPoint;
  glm::vec3 nextDraggedPoint;
};

struct UserInputController::Impl {
  Scene* scene;
  VkSceneRenderer* renderer;
  MouseButtonState mouseLeftButtonState;
  MouseButtonState mouseMiddleButtonState;
  MouseButtonState mouseRightButtonState;
};

UserInputController::UserInputController(Scene* scene,
                                         VkSceneRenderer* renderer) {
  impl_ = new Impl{
    .scene = scene,
    .renderer = renderer
  };
}

UserInputController::~UserInputController() { delete impl_; }

void UserInputController::mouseLeftButtonDown(int x, int y) {
  impl_->mouseLeftButtonState.down = true;
  impl_->mouseLeftButtonState.xDown = x;
  impl_->mouseLeftButtonState.yDown = y;
  impl_->mouseLeftButtonState.xPrev = x;
  impl_->mouseLeftButtonState.yPrev = y;
}

void UserInputController::mouseMiddleButtonDown(int x, int y) {
  impl_->mouseMiddleButtonState.down = true;
  impl_->mouseMiddleButtonState.xDown = x;
  impl_->mouseMiddleButtonState.yDown = y;
  impl_->mouseMiddleButtonState.xPrev = x;
  impl_->mouseMiddleButtonState.yPrev = y;
}

void UserInputController::mouseRightButtonDown(int x, int y) {
  impl_->mouseRightButtonState.down = true;
  impl_->mouseRightButtonState.xDown = x;
  impl_->mouseRightButtonState.yDown = y;
  impl_->mouseRightButtonState.xPrev = x;
  impl_->mouseRightButtonState.yPrev = y;
}

void UserInputController::mouseLeftButtonUp(int x, int y) {
  if (!impl_->mouseLeftButtonState.down) return;

  impl_->mouseLeftButtonState.down = false;

  bool beClicked = impl_->mouseLeftButtonState.xDown == x &&
                   impl_->mouseLeftButtonState.yDown == y;
}

void UserInputController::mouseMiddleButtonUp(int x, int y) {
  if (!impl_->mouseMiddleButtonState.down) return;

  impl_->mouseMiddleButtonState.down = false;

  bool beClicked = impl_->mouseMiddleButtonState.xDown == x &&
                   impl_->mouseMiddleButtonState.yDown == y;
}

void UserInputController::mouseRightButtonUp(int x, int y) {
  if (!impl_->mouseRightButtonState.down) return;

  impl_->mouseRightButtonState.down = false;

  bool beClicked = impl_->mouseRightButtonState.xDown == x &&
                   impl_->mouseRightButtonState.yDown == y;
}

void UserInputController::mouseMove(int x, int y) {
  if (impl_->mouseLeftButtonState.down) {
    const int dx = x - impl_->mouseLeftButtonState.xPrev;
    const int dy = y - impl_->mouseLeftButtonState.yPrev;

    impl_->scene->TranslateCamera(x, y, dx, dy);
    impl_->renderer->Update(impl_->scene);

    impl_->mouseLeftButtonState.xPrev = x;
    impl_->mouseLeftButtonState.yPrev = y;
  } else if (impl_->mouseMiddleButtonState.down) {
    const int dx = x - impl_->mouseMiddleButtonState.xPrev;
    const int dy = y - impl_->mouseMiddleButtonState.yPrev;

    // ...

    impl_->mouseMiddleButtonState.xPrev = x;
    impl_->mouseMiddleButtonState.yPrev = y;
  } else if (impl_->mouseRightButtonState.down) {
    const int dx = x - impl_->mouseRightButtonState.xPrev;
    const int dy = y - impl_->mouseRightButtonState.yPrev;

    impl_->scene->RotateCamera(x, y, dx, dy);
    impl_->renderer->Update(impl_->scene);

    impl_->mouseRightButtonState.xPrev = x;
    impl_->mouseRightButtonState.yPrev = y;
  }
}

void UserInputController::mouseWheelRotate(int numDegrees) {
  impl_->scene->ZoomCamera(numDegrees / 30.0f);
  impl_->renderer->Update(impl_->scene);
}

void UserInputController::keyPressed(int key) {}

void UserInputController::keyReleased(int key) {}
