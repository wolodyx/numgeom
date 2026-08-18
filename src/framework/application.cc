#include "numgeom/application.h"

#include <format>
#include <iostream>
#include <map>

#include "numgeom/alignedboundbox.h"
#include "numgeom/drawable.h"
#include "numgeom/fgtext.h"
#include "numgeom/iteratorimpl.hpp"
#include "numgeom/ray.h"
#include "numgeom/scene.h"
#include "numgeom/sceneobject_mesh.h"
#include "numgeom/scenewidget_axisindicator.h"
#include "numgeom/trimesh.h"
#include "numgeom/vkscenerenderer.h"

#include "fgimage.h"
#include "intersection.h"
#include "trackedobjectlist.h"
#include "trackedobjectdict.h"
#include "vkutilities.h"
#include "workplaneattributes.h"

class ApplicationImpl {
 public:
  ~ApplicationImpl() {
    delete renderer_;
  }

 public:
  TrackedObjectList scenes_;
  TrackedObjectList fg_objects_;
  VkSceneRenderer* renderer_ = nullptr;
  SelectionMode selection_mode_ = SelectionMode::Disable;
  WorkplaneAttributes workplane_attributes_;
};

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
  impl_ = new ApplicationImpl();
  impl_->renderer_ = new VkSceneRenderer(this);
}

Application::~Application() { delete impl_; }

bool Application::Update(Scene* scene, bool force) {
  if (!scene)
    return false;
  impl_->renderer_->Update(scene, force);
  return true;
}

VkSceneRenderer* Application::GetRenderer() { return impl_->renderer_; }

Scene* Application::AddScene() {
  Scene* new_scene = new Scene{};
  impl_->scenes_.Insert(new_scene);
  new_scene->SetSelectionMode(impl_->selection_mode_);
  return new_scene;
}

bool Application::RemoveScene(Scene* scene) {
  if (!scene)
    return false;
  return impl_->scenes_.Remove(scene);
}

FgObject* Application::AddFgImage(const std::string& image_filename) {
  auto fg_image = FgImage::Make(image_filename);
  if (!fg_image)
    return nullptr;
  impl_->fg_objects_.Insert(fg_image);
  return fg_image;
}

FgObject* Application::AddFgImage(const unsigned char* image_data,
                                 size_t image_data_size) {
  auto fg_image = FgImage::Make(image_data, image_data_size);
  if (!fg_image)
    return nullptr;
  impl_->fg_objects_.Insert(fg_image);
  return fg_image;
}

FgText* Application::AddFgText(const std::string& text) {
  if (text.empty())
    return nullptr;
  auto fg_text = FgText::Make(text);
  impl_->fg_objects_.Insert(fg_text);
  return fg_text;
}

bool Application::Remove(Scene*, const FgObject*) {
  return false;
}

void Application::Sync() {
  impl_->scenes_.Synch();
  impl_->fg_objects_.Synch();
}

bool Application::SetSampleCount(SampleCount count) {
  if (count == SampleCount::Bits_None)
    return false;
  VkSampleCountFlagBits max_count = impl_->renderer_->GetMaxUsableSampleCount();
  VkSampleCountFlagBits desired_count = ToVkSampleCountFlagBits(count);
  bool in_the_range = static_cast<int>(desired_count) <= static_cast<int>(max_count);
  if (!in_the_range)
    return false;
  VkSampleCountFlagBits next_count = (in_the_range ? desired_count : max_count);
  return impl_->renderer_->SetSampleCount(next_count);
}

SampleCount Application::GetSampleCount() const {
  return ToSampleCount(impl_->renderer_->GetSampleCount());
}

namespace {
Ray ScreenPosToRay(const Scene* scene, const glm::ivec2& screen_pos) {
  const glm::mat4 view = scene->GetViewMatrix();
  const glm::mat4 proj = scene->GetProjectionMatrix();
  const glm::uvec2 screen_size = scene->GetScreenSize();

  if (screen_size.x <= 0 || screen_size.y <= 0)
    return Ray(scene->CameraPosition(), glm::vec3(0.0f, 0.0f, 1.0f));

  const float fx = static_cast<float>(screen_pos.x);
  const float fy = static_cast<float>(screen_pos.y);
  const float width = static_cast<float>(screen_size.x);
  const float height = static_cast<float>(screen_size.y);
  // Mind: Vulkan clip space Y points down (top-left origin), so window fy=0
  // (top) maps to NDC y = +1. This must match GetWorldPointOnViewParallelPlane
  // in userinputcontroller.cc so a picked/clicked point lands on the same
  // view-parallel plane used while dragging.
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
  const glm::vec3 world_near = glm::vec3(world_near4) / world_near4.w;
  const glm::vec3 world_far = glm::vec3(world_far4) / world_far4.w;
  const glm::vec3 direction = glm::normalize(world_far - world_near);
  return Ray(world_near, direction);
}
}

Drawable* Application::Pick(Scene* scene, int x, int y,
                            glm::vec3* picked_point) const {
  auto id = impl_->renderer_->GetObjectId(scene, x, y);
  if (id == 0) return nullptr;
  Drawable* picked_item = nullptr;
  for (auto o : scene->Objects()) {
    for (auto d : o->Drawables()) {
      if (d->GetId() == id) {
        picked_item = d;
        break;
      }
    }
    if (picked_item) break;
  }
  if (picked_item && picked_point) {
    Ray ray = ScreenPosToRay(scene, glm::ivec2(x, y));
    *picked_point = IntersectRayWithDrawable(ray, dynamic_cast<Drawable2*>(picked_item));
  }
  return picked_item;
}

SelectionMode Application::GetSelectionMode() const {
  return impl_->selection_mode_;
}

void Application::SetSelectionMode(SelectionMode selection_mode) {
  impl_->selection_mode_ = selection_mode;
  for (Scene* s : GetObjects<Scene>(impl_->scenes_)) {
    s->SetSelectionMode(selection_mode);
  }
}

const WorkplaneAttributes* Application::GetWorkplaneAttributes() const {
  return &impl_->workplane_attributes_;
}
