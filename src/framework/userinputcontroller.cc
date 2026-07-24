#include "numgeom/userinputcontroller.h"

#include <vector>

#include "numgeom/application.h"
#include "numgeom/drawable.h"
#include "numgeom/scene.h"
#include "numgeom/vkscenerenderer.h"

#include "glm/gtc/matrix_inverse.hpp"

enum class SelectionMode { Single, Multiple, DraggingOnly };

struct PickingMode
{
  static inline const std::function<bool(const Drawable*)> EmptyFilter =
    [](const Drawable*) { return false; };

  PickingMode() {
    sel_mode = SelectionMode::Single;
    filter = EmptyFilter;
  }

  SelectionMode sel_mode;
  std::function<bool(const Drawable*)> filter;
};

struct MouseButtonState {
  bool down = false;
  int xDown = 0, yDown = 0;
  int xPrev = 0, yPrev = 0;
  Drawable* selected_item = nullptr;
  // glm::vec3 clicked_point;
  // glm::vec3 prev_dragged_point;
  // glm::vec3 next_dragged_point;
};

struct UserInputController::Impl {
  Application* app;
  Scene* scene;
  MouseButtonState mouseLeftButtonState;
  MouseButtonState mouseMiddleButtonState;
  MouseButtonState mouseRightButtonState;
  std::vector<Drawable*> selected_items;
  PickingMode picking_mode;
};

UserInputController::UserInputController(Application* app, Scene* scene) {
  impl_ = new Impl{
    .app = app,
    .scene = scene,
  };
}

UserInputController::~UserInputController() {}

void UserInputController::mouseLeftButtonDown(int x, int y) {
  impl_->mouseLeftButtonState.down = true;
  impl_->mouseLeftButtonState.xDown = x;
  impl_->mouseLeftButtonState.yDown = y;
  impl_->mouseLeftButtonState.xPrev = x;
  impl_->mouseLeftButtonState.yPrev = y;
  Drawable* item = impl_->app->Pick(impl_->scene, x, y);
  impl_->mouseLeftButtonState.selected_item = item;
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

  // The event of a left-click on the scene, not on the object.
  if (beClicked && !impl_->mouseLeftButtonState.selected_item) {
    return;
  }

  // The event of a left-click on an object.
  std::vector<Drawable*> selected_objs, deselected_objs;
  if (beClicked) {
    auto it = std::find(impl_->selected_items.begin(),
                        impl_->selected_items.end(),
                        impl_->mouseLeftButtonState.selected_item);

    if (it != impl_->selected_items.end()) {
      impl_->selected_items.erase(it);
      impl_->mouseLeftButtonState.selected_item->Deselect();
      deselected_objs.push_back(impl_->mouseLeftButtonState.selected_item);
    }
    else {
      if (impl_->picking_mode.sel_mode == SelectionMode::Single) {
        for(auto item : impl_->selected_items)
          item->Deselect();
        deselected_objs = impl_->selected_items;
        impl_->selected_items.clear();
      }

      auto picked_item = impl_->mouseLeftButtonState.selected_item;
      if (picked_item && impl_->picking_mode.sel_mode != SelectionMode::DraggingOnly) {
        picked_item->Select();
        impl_->selected_items.push_back(picked_item);
        selected_objs.push_back(picked_item);
      }
    }

    VkSceneRenderer* renderer = impl_->app->GetRenderer();
    renderer->Update(impl_->scene);
  }
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
  VkSceneRenderer* renderer = impl_->app->GetRenderer();

  if (impl_->mouseLeftButtonState.down) {
    const int dx = x - impl_->mouseLeftButtonState.xPrev;
    const int dy = y - impl_->mouseLeftButtonState.yPrev;

    impl_->scene->TranslateCamera(x, y, dx, dy);
    renderer->Update(impl_->scene);

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
    renderer->Update(impl_->scene);

    impl_->mouseRightButtonState.xPrev = x;
    impl_->mouseRightButtonState.yPrev = y;
  }
}

void UserInputController::mouseWheelRotate(int numDegrees) {
  VkSceneRenderer* renderer = impl_->app->GetRenderer();
  impl_->scene->ZoomCamera(numDegrees / 30.0f);
  renderer->Update(impl_->scene);
}

void UserInputController::keyPressed(int key) {}

void UserInputController::keyReleased(int key) {}
