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
}

Application::~Application() { delete impl_; }

void Application::Update() { impl_->renderer->update(); }

VkSceneRenderer* Application::GetRenderer() { return impl_->renderer; }

const Scene* Application::GetActiveScene() const { return impl_->scenes_.front(); }

Scene* Application::GetActiveScene() { return impl_->scenes_.front(); }

const Scene* Application::GetForegroundScene() const { return impl_->scenes_.back(); }

Scene* Application::GetForegroundScene() { return impl_->scenes_.back(); }

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

void Application::AddAxisIndicator() {
  impl_->scenes_.back()->AddObject<SceneWidget_AxisIndicator>();
}
