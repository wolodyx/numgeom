#include "numgeom/scene.h"

#include "numgeom/fgtext.h"
#include "numgeom/iteratorimpl.hpp"
#include "numgeom/sceneobject.h"

#include "trackedobjectlist.h"

#include "camera.h"

class SceneImpl {
 public:
  std::map<FgObject*,glm::ivec2> fgobjects_positions;
  Camera camera_;
  TrackedObjectList objects_;
  uint64_t vulkan_surface_ = 0;
  bool workplane_enabled_;
};

Scene::Scene() {
  impl_ = new SceneImpl{};
  this->AddSubObjects(&impl_->objects_);
  impl_->camera_.SetBoundBoxFunction(
    [this](){ return this->GetBoundBox(); });
}

Scene::~Scene() {
  delete impl_;
}

AlignedBoundBox Scene::GetBoundBox() const {
  AlignedBoundBox box;
  for (SceneObject* o : this->VisibleObjects()) {
    box.Expand(o->GetBoundBox());
  }
  return box;
}

void Scene::Clear() {
  if(impl_->objects_.IsEmpty() && impl_->fgobjects_positions.empty() &&
     impl_->fgobjects_positions.empty())
    return;
  this->SetDirty();
  impl_->objects_.Clear();
  impl_->fgobjects_positions.clear();
  impl_->fgobjects_positions.clear();
}

Iterator<SceneObject*> Scene::Objects() const {
  return GetObjects<SceneObject>(impl_->objects_);
}

Iterator<SceneObject*> Scene::VisibleObjects() const {
  Iterator<SceneObject*> it = GetObjects<SceneObject>(impl_->objects_);
  return Filter(it, [](SceneObject* o){
                      return o->IsVisible();
                    });
}

void Scene::EnableWorkplane(bool enable) {
  if (impl_->workplane_enabled_ == enable) return;
  impl_->workplane_enabled_ = enable;
  this->SetDirty();
}

void Scene::DisableWorkplane() {
  EnableWorkplane(false);
}

bool Scene::IsWorkplaneEnabled() const {
  return impl_->workplane_enabled_;
}

void Scene::AddObject(SceneObject* object) {
  impl_->objects_.Insert(object);
}

glm::mat4 Scene::GetViewMatrix() const {
  return impl_->camera_.GetViewMatrix();
}

glm::mat4 Scene::GetProjectionMatrix() const {
  return impl_->camera_.GetProjectionMatrix(this->GetBoundBox());
}

glm::uvec2 Scene::GetScreenSize() const {
  return impl_->camera_.GetScreenSize();
}

void Scene::FitScene() {
  AlignedBoundBox box = this->GetBoundBox();
  if (box.IsEmpty())
    box = AlignedBoundBox{{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 1.0f}};
  box.Scale(1.3);
  impl_->camera_.FitBox(box);
}

void Scene::ZoomCamera(float k) {
  impl_->camera_.Zoom(k);
}

glm::vec3 Scene::CameraPosition() const {
  return impl_->camera_.GetPosition();
}

void Scene::TranslateCamera(int x, int y, int dx, int dy) {
  if (dx == 0 && dy == 0) return;
  impl_->camera_.Translate(glm::ivec2{dx,dy});
}

void Scene::RotateCamera(int x, int y, int dx, int dy) {
  if (dx == 0 && dy == 0) return;
  AlignedBoundBox box = this->GetBoundBox();
  if (box.IsEmpty())
    return;
  impl_->camera_.SetPivotPoint(box.GetCenter());
  glm::vec2 screen_offset(static_cast<float>(dx), static_cast<float>(dy));
  impl_->camera_.RotateAroundPivot(screen_offset);
}

void Scene::OrientCamera(const OrthoBasis<float>& ortho_basis) {
  impl_->camera_.Orient(ortho_basis);
  this->FitScene();
}

bool Scene::HasFgObjects() const {
  return !impl_->fgobjects_positions.empty();
}

Iterator<FgObject*> Scene::GetFgObjects() const {
  auto it = new IteratorImpl_StdMapKey<FgObject*,glm::ivec2>(
      impl_->fgobjects_positions.begin(),
      impl_->fgobjects_positions.end());
  return Iterator<FgObject*>(it);
}

glm::ivec2 Scene::GetScreenPosition(const FgObject* fg) const {
  auto it = impl_->fgobjects_positions.find(const_cast<FgObject*>(fg));
  if (it == impl_->fgobjects_positions.end())
    return glm::ivec2{};
  return it->second;
}

void Scene::AddFgObject(FgObject* fg, const glm::ivec2& screen_position) {
  impl_->fgobjects_positions[fg] = screen_position;
  this->SetDirty();
}

void Scene::InitializeGraphics(
    std::function<std::tuple<uint32_t, uint32_t>()> get_viewport_size,
    uint64_t vulkan_surface) {
  impl_->vulkan_surface_ = vulkan_surface;
  impl_->camera_.SetViewportSizeFunction(get_viewport_size);
}

uint64_t Scene::GetVulkanSurface() const {
  return impl_->vulkan_surface_;
}
