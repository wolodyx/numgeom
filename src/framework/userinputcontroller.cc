#include "numgeom/userinputcontroller.h"

#include <cmath>
#include <vector>

#include "numgeom/application.h"
#include "numgeom/drawable.h"
#include "numgeom/scene.h"
#include "numgeom/scenewidget.h"
#include "numgeom/vkscenerenderer.h"

#include "glm/gtc/matrix_inverse.hpp"

struct MouseButtonState {
  bool down = false;
  int xDown = 0, yDown = 0;
  int xPrev = 0, yPrev = 0;
  Drawable* selected_item = nullptr;
  glm::vec3 clicked_point;
  glm::vec3 prev_dragged_point;
  glm::vec3 next_dragged_point;
};

struct UserInputController::Impl {
  static inline const std::function<bool(const Drawable*)> EmptyFilter =
    [](const Drawable*) { return false; };

  Application* app;
  Scene* scene;
  MouseButtonState mouseLeftButtonState;
  MouseButtonState mouseMiddleButtonState;
  MouseButtonState mouseRightButtonState;
  std::vector<Drawable*> selected_items;
  std::function<bool(const Drawable*)> filter = EmptyFilter;
};

UserInputController::UserInputController(Application* app, Scene* scene) {
  impl_ = new Impl{
    .app = app,
    .scene = scene,
  };
}

UserInputController::~UserInputController() {}

void UserInputController::MouseLeftButtonDown(int x, int y) {
  impl_->mouseLeftButtonState.down = true;
  impl_->mouseLeftButtonState.xDown = x;
  impl_->mouseLeftButtonState.yDown = y;
  impl_->mouseLeftButtonState.xPrev = x;
  impl_->mouseLeftButtonState.yPrev = y;
  glm::vec3 picked_point;
  Drawable* item = impl_->app->Pick(impl_->scene, x, y, &picked_point);
  impl_->mouseLeftButtonState.selected_item = item;
  if (item) {
    impl_->mouseLeftButtonState.clicked_point = picked_point;
    impl_->mouseLeftButtonState.next_dragged_point = picked_point;
    impl_->mouseLeftButtonState.prev_dragged_point = picked_point;
  }
}

void UserInputController::MouseMiddleButtonDown(int x, int y) {
  impl_->mouseMiddleButtonState.down = true;
  impl_->mouseMiddleButtonState.xDown = x;
  impl_->mouseMiddleButtonState.yDown = y;
  impl_->mouseMiddleButtonState.xPrev = x;
  impl_->mouseMiddleButtonState.yPrev = y;
}

void UserInputController::MouseRightButtonDown(int x, int y) {
  impl_->mouseRightButtonState.down = true;
  impl_->mouseRightButtonState.xDown = x;
  impl_->mouseRightButtonState.yDown = y;
  impl_->mouseRightButtonState.xPrev = x;
  impl_->mouseRightButtonState.yPrev = y;
}

void UserInputController::MouseLeftButtonUp(int x, int y) {
  if (!impl_->mouseLeftButtonState.down) return;

  impl_->mouseLeftButtonState.down = false;

  bool be_clicked = impl_->mouseLeftButtonState.xDown == x &&
                   impl_->mouseLeftButtonState.yDown == y;

  if (!be_clicked) return;

  // The event of a left-click on an object.

  Drawable* selected_item = impl_->mouseLeftButtonState.selected_item;
  SelectionMode selection_mode = impl_->scene->GetSelectionMode();
  std::vector<Drawable*> selected_objs, deselected_objs;

  if (!selected_item) {
    for(auto item : impl_->selected_items)
      item->Deselect();
    deselected_objs = impl_->selected_items;
    impl_->selected_items.clear();
  } else if (selection_mode == SelectionMode::Disable) {
    for(auto item : impl_->selected_items)
      item->Deselect();
    deselected_objs = impl_->selected_items;
    impl_->selected_items.clear();
  } else if (selection_mode == SelectionMode::Single) {
    auto it = std::find(impl_->selected_items.begin(),
                        impl_->selected_items.end(), selected_item);
    if (it != impl_->selected_items.end()) {
      impl_->selected_items.erase(it);
      selected_item->Deselect();
      deselected_objs.push_back(selected_item);
    } else {
      deselected_objs = impl_->selected_items;
      selected_objs = {selected_item};
      for(auto item : impl_->selected_items)
        item->Deselect();
      impl_->selected_items.clear();
      selected_item->Select();
      impl_->selected_items.push_back(selected_item);
    }
  } else if (selection_mode == SelectionMode::Multiple) {
    auto it = std::find(impl_->selected_items.begin(),
                        impl_->selected_items.end(), selected_item);
    if (it != impl_->selected_items.end()) {
      impl_->selected_items.erase(it);
      selected_item->Deselect();
      deselected_objs.push_back(selected_item);
    } else {
      selected_item->Select();
      impl_->selected_items.push_back(selected_item);
      selected_objs.push_back(selected_item);
    }
  }
  VkSceneRenderer* renderer = impl_->app->GetRenderer();
  renderer->Update(impl_->scene);
}

void UserInputController::MouseMiddleButtonUp(int x, int y) {
  if (!impl_->mouseMiddleButtonState.down) return;

  impl_->mouseMiddleButtonState.down = false;

  bool beClicked = impl_->mouseMiddleButtonState.xDown == x &&
                   impl_->mouseMiddleButtonState.yDown == y;
}

void UserInputController::MouseRightButtonUp(int x, int y) {
  if (!impl_->mouseRightButtonState.down) return;

  impl_->mouseRightButtonState.down = false;

  bool beClicked = impl_->mouseRightButtonState.xDown == x &&
                   impl_->mouseRightButtonState.yDown == y;
}

namespace {
bool IsInteractiveItem(const Drawable* item) {
  if (!item) return false;
  auto w = dynamic_cast<const SceneWidget*>(item->GetParent());
  if (!w) return false;
  return item->IsPickable();
}

//! Вычисляет точку в мировых координатах по экранным координатам `screen_pos`
//! такую, что вектор исходящей из `prev_point` до этой точки параллелен
//! плоскости камеры из `scene`.
glm::vec3 GetWorldPointOnViewParallelPlane(Scene* scene,
                                           const glm::vec3& prev_point,
                                           const glm::ivec2& screen_pos) {
  const glm::mat4 view = scene->GetViewMatrix();
  const glm::mat4 proj = scene->GetProjectionMatrix();
  const glm::uvec2 screen_size = scene->GetScreenSize();

  if (screen_size.x <= 0 || screen_size.y <= 0)
    return prev_point;

  const float fx = static_cast<float>(screen_pos.x);
  const float fy = static_cast<float>(screen_pos.y);
  const float width = static_cast<float>(screen_size.x);
  const float height = static_cast<float>(screen_size.y);
  // Vulkan clip space has Y pointing down (top-left origin), so a window
  // coordinate fy=0 (top) maps to NDC y = +1, while a positive fov matrix
  // (Y-up) would map it to -1. Using the Y-down mapping keeps the pick/drag
  // math aligned with the on-screen (Vulkan) image.
  const glm::vec4 ndc_near{
      (2.0f * fx) / width - 1.0f,
      (2.0f * fy) / height - 1.0f,
      0.0f,
      1.0f};
  const glm::vec4 ndc_far{
      (2.0f * fx) / width - 1.0f,
      (2.0f * fy) / height - 1.0f,
      1.0f,
      1.0f};
  const glm::mat4 inv_view_proj = glm::inverse(proj * view);
  const glm::vec4 world_near4 = inv_view_proj * ndc_near;
  const glm::vec4 world_far4 = inv_view_proj * ndc_far;
  const glm::vec3 ray_origin = glm::vec3(world_near4) / world_near4.w;
  const glm::vec3 ray_dir = glm::normalize(
      glm::vec3(world_far4) / world_far4.w - ray_origin);

  // The plane parallel to the camera view passing through `prev_point`
  // has the camera forward direction as its normal. The forward direction
  // is the third row of the view matrix rotation part.
  const glm::vec3 plane_normal =
      glm::normalize(glm::vec3(view[0][2], view[1][2], view[2][2]));

  const float denom = glm::dot(plane_normal, ray_dir);
  if (std::abs(denom) < 1e-6f)
    return prev_point;

  const float t = glm::dot(plane_normal, prev_point - ray_origin) / denom;
  return ray_origin + t * ray_dir;
}
}

void UserInputController::MouseMove(int x, int y) {
  bool is_scene_dirty = false;
  glm::ivec2 screen_pos{x, y};
  if (impl_->mouseLeftButtonState.down) {
    MouseButtonState& mbs = impl_->mouseLeftButtonState;
    if (IsInteractiveItem(mbs.selected_item)) {
      auto* selected_item = mbs.selected_item;
      auto* widget = dynamic_cast<SceneWidget*>(selected_item->GetParent());
      mbs.prev_dragged_point = mbs.next_dragged_point;
      mbs.next_dragged_point = GetWorldPointOnViewParallelPlane(impl_->scene, mbs.prev_dragged_point, screen_pos);
      auto dragging_dir = mbs.next_dragged_point - mbs.prev_dragged_point;
      widget->Drag(selected_item, dragging_dir);
    } else {
      int dx = x - mbs.xPrev;
      int dy = y - mbs.yPrev;
      impl_->scene->TranslateCamera(x, y, dx, dy);
    }
    is_scene_dirty = true;
    mbs.xPrev = x;
    mbs.yPrev = y;
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
    is_scene_dirty = true;

    impl_->mouseRightButtonState.xPrev = x;
    impl_->mouseRightButtonState.yPrev = y;
  }

  if (is_scene_dirty) {
  VkSceneRenderer* renderer = impl_->app->GetRenderer();
    renderer->Update(impl_->scene);
  }
}

void UserInputController::MouseWheelRotate(int numDegrees) {
  VkSceneRenderer* renderer = impl_->app->GetRenderer();
  impl_->scene->ZoomCamera(numDegrees / 30.0f);
  renderer->Update(impl_->scene);
}

void UserInputController::KeyPressed(int key) {}

void UserInputController::KeyReleased(int key) {}
