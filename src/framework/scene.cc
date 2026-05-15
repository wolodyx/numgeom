#include "numgeom/scene.h"

#include "numgeom/iteratorimpl.hpp"
#include "numgeom/sceneobject.h"
#include "numgeom/screentext.h"

#include "sceneinner.h"
#include "scenestate.h"

Scene::Scene(const std::string& name) {
  state_ = new State();
  state_->name_ = name;
  state_->camera_.SetBoundBoxFunction(
    [this](){ return this->GetBoundBox(); });
}

Scene::~Scene() {
  delete state_;
}

std::string Scene::GetName() const {
  return state_->name_;
}

AlignedBoundBox Scene::GetBoundBox() const {
  AlignedBoundBox box;
  for (SceneObject* o : state_->objects_) {
    box.Expand(o->GetBoundBox());
  }
  return box;
}

void Scene::Clear() {
  if(state_->objects_.empty())
    return;
  state_->has_changes_ = true;
  for (SceneObject* o : state_->objects_) {
    delete o;
  }
  state_->objects_.clear();
}

bool Scene::HasChanges() const {
  if (state_->has_changes_)
    return true;
  for (SceneObject* o : state_->objects_) {
    if (o->HasChanges())
      return true;
  }
  return false;
}

void Scene::ClearChanges() {
  for (SceneObject* o : state_->objects_)
    o->ClearChanges();
  state_->has_changes_ = false;
}

Iterator<SceneObject*> Scene::Objects() const {
  typedef std::list<SceneObject*>::const_iterator StdIterType;
  auto it_impl = new IteratorImpl_StdIterator<StdIterType>(
      state_->objects_.begin(),
      state_->objects_.end());
  return Iterator<SceneObject*>(it_impl);
}

void Scene::AddObject(SceneObject* object) {
  state_->objects_.push_back(object);
  state_->has_changes_ = true;
}

glm::mat4 Scene::GetViewMatrix() const {
  return state_->camera_.GetViewMatrix();
}

glm::mat4 Scene::GetProjectionMatrix() const {
  return state_->camera_.GetProjectionMatrix(this->GetBoundBox());
}

glm::uvec2 Scene::GetScreenSize() const {
  return state_->camera_.GetScreenSize();
}

void Scene::FitScene() {
  AlignedBoundBox box = this->GetBoundBox();
  box.Scale(1.3);
  state_->camera_.FitBox(box);
}

void Scene::ZoomCamera(float k) {
  state_->camera_.Zoom(k);
}

glm::vec3 Scene::CameraPosition() const {
  return state_->camera_.GetPosition();
}

void Scene::TranslateCamera(int x, int y, int dx, int dy) {
  if (dx == 0 && dy == 0) return;
  state_->camera_.Translate(glm::ivec2{dx,dy});
}

void Scene::RotateCamera(int x, int y, int dx, int dy) {
  if (dx == 0 && dy == 0) return;
  AlignedBoundBox box = this->GetBoundBox();
  if (box.IsEmpty())
    return;
  state_->camera_.SetPivotPoint(box.GetCenter());
  glm::vec2 screen_offset(static_cast<float>(dx), static_cast<float>(dy));
  state_->camera_.RotateAroundPivot(screen_offset);
}

void Scene::OrientCamera(const OrthoBasis<float>& ortho_basis) {
  state_->camera_.Orient(ortho_basis);
  this->FitScene();
}

void Scene::SetViewportSizeFunction(std::function<std::tuple<uint32_t,uint32_t>()> func) {
  state_->camera_.SetViewportSizeFunction(func);
}

void Scene::SetLogo(const std::string& image_filename,
                          const glm::ivec2& screen_position) {
  state_->logo = Logo(image_filename, screen_position);
}

void Scene::SetLogo(const unsigned char* image_data,
                          size_t image_data_size,
                          const glm::ivec2& screen_position) {
  state_->logo = Logo(image_data, image_data_size, screen_position);
}

ScreenText* Scene::SetText(const std::string& text) {
  auto o = new ScreenText(text, glm::ivec2(0,0));
  state_->screen_text_objects_.push_back(o);
  return o;
}

Scene::Inner* Scene::GetInnerInterface() {
  if (state_->inner_interface_ == nullptr)
    state_->inner_interface_ = new Inner(state_);
  return state_->inner_interface_;
}

const Scene::Inner* Scene::GetInnerInterface() const {
  if (state_->inner_interface_ == nullptr)
    state_->inner_interface_ = new Inner(state_);
  return state_->inner_interface_;
}
