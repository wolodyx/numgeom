#include "numgeom/application.h"

#include <format>
#include <iostream>

#include "numgeom/alignedboundbox.h"
#include "numgeom/sceneobject_mesh.h"
#include "numgeom/scenewidget_axisindicator.h"
#include "numgeom/trimesh.h"
#include "numgeom/vkscenerenderer.h"

#include "applicationinner.h"
#include "applicationstate.h"

namespace {
AlignedBoundBox ComputeBoundBox(CTriMesh::Ptr scene) {
  assert(scene != nullptr);
  AlignedBoundBox box;
  for (size_t i = 0; i < scene->NbCells(); ++i) {
    const auto& cell = scene->GetCell(i);
    for (size_t j = 0; j < 3; ++j) {
      auto pt = scene->GetNode(cell.GetNodeIndex(j));
      box.Expand(pt);
    }
  }

  glm::vec3 center = box.GetCenter();
  glm::vec3 size = box.GetSize();
  float maxLenght = std::max(size.x, std::max(size.y, size.z));
  for (int i = 0; i < 3; ++i) {
    float& cMin = *(&box.min_.x + i);
    float& cMax = *(&box.max_.x + i);
    if (cMin == cMax) {
      cMin = *(&center.x + i) - 0.5 * maxLenght;
      cMax = *(&center.x + i) + 0.5 * maxLenght;
    }
  }
  return box;
}
}  // namespace

Application::Application(int argc, char* argv[]) {
  impl_ = new Application::State();
  impl_->renderer = new VkSceneRenderer(this);
  impl_->camera.SetBoundBoxFunction(
    [this](){ return this->impl_->scene.GetBoundBox(); });
}

Application::~Application() { delete impl_; }

void Application::FitScene() {
  AlignedBoundBox box = impl_->scene.GetBoundBox();
  box.Scale(1.3);
  impl_->camera.FitBox(box);
  this->Update();
}

void Application::ZoomCamera(float k) {
  impl_->camera.Zoom(k);
  this->Update();
}

glm::vec3 Application::CameraPosition() const {
  return impl_->camera.GetPosition();
}

void Application::TranslateCamera(int x, int y, int dx, int dy) {
  if (dx == 0 && dy == 0) return;
  impl_->camera.Translate(glm::ivec2{dx,dy});
  this->Update();
}

void Application::RotateCamera(int x, int y, int dx, int dy) {
  if (dx == 0 && dy == 0) return;
  AlignedBoundBox box = impl_->scene.GetBoundBox();
  if (box.IsEmpty())
    return;
  impl_->camera.SetPivotPoint(box.GetCenter());
  glm::vec2 screen_offset(static_cast<float>(dx), static_cast<float>(dy));
  impl_->camera.RotateAroundPivot(screen_offset);
  this->Update();
}

void Application::OrientCamera(const OrthoBasis<float>& ortho_basis) {
  impl_->camera.Orient(ortho_basis);
  this->FitScene();
}

void Application::Update() { impl_->renderer->update(); }

VkSceneRenderer* Application::GetRenderer() { return impl_->renderer; }

const Scene& Application::GetScene() const { return impl_->scene; }

Scene& Application::GetScene() { return impl_->scene; }

void Application::ClearScene() {
  impl_->scene.Clear();
  this->Update();
}

void Application::SetViewportSizeFunction(std::function<std::tuple<uint32_t,uint32_t>()> func) {
  impl_->camera.SetViewportSizeFunction(func);
}

void Application::SetLogo(const std::string& image_filename,
                          const glm::ivec2& screen_position) {
  impl_->logo = Logo(image_filename, screen_position);
}

void Application::SetLogo(const unsigned char* image_data,
                          size_t image_data_size,
                          const glm::ivec2& screen_position) {
  impl_->logo = Logo(image_data, image_data_size, screen_position);
}

Application::Inner* Application::GetInnerInterface() {
  if (impl_->inner_interface_ == nullptr)
    impl_->inner_interface_ = new Inner(impl_);
  return impl_->inner_interface_;
}

const Application::Inner* Application::GetInnerInterface() const {
  if (impl_->inner_interface_ == nullptr)
    impl_->inner_interface_ = new Inner(impl_);
  return impl_->inner_interface_;
}

ScreenText* Application::SetText(const std::string& text) {
  auto o = new ScreenText(text, glm::ivec2(0,0));
  impl_->screen_text_objects_.push_back(o);
  this->Update();
  return o;
}
