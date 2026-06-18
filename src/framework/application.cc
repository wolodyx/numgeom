#include "numgeom/application.h"

#include <format>
#include <iostream>
#include <map>

#include "numgeom/alignedboundbox.h"
#include "numgeom/scene.h"
#include "numgeom/sceneobject_mesh.h"
#include "numgeom/scenewidget_axisindicator.h"
#include "numgeom/trimesh.h"
#include "numgeom/vkscenerenderer.h"

#include "fgimage.h"
#include "trackedobjectlist.h"
#include "trackedobjectdict.h"

class ApplicationImpl {
 public:
  ~ApplicationImpl() {
    delete renderer_;
  }

 public:
  TrackedObjectDict scenes_;
  std::map<Scene*,Scene*> foreground2background_;
  Scene* active_scene_ = nullptr;
  TrackedObjectList fg_images_;
  TrackedObjectList screen_texts_;
  VkSceneRenderer* renderer_ = nullptr;
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

bool Application::Update(Scene* scene) {
  if (!scene)
    return false;
  impl_->renderer_->Update(scene);
  return true;
}

VkSceneRenderer* Application::GetRenderer() { return impl_->renderer_; }

Scene* Application::AddAxisIndicator() {
  if (!impl_->active_scene_)
    return nullptr;
  auto foreground_scene = AddScene("axis-indicator", impl_->active_scene_);
  if (!foreground_scene)
    return nullptr;
  foreground_scene->AddObject<SceneWidget_AxisIndicator>();
  return foreground_scene;
}

Scene* Application::AddScene(const std::string& new_scene_name,
                             Scene* background_scene) {
  // Check input arguments.
  if (new_scene_name.empty())
    return nullptr;
  if (impl_->scenes_.Contains(new_scene_name))
    return nullptr;
  if (background_scene && !impl_->scenes_.Contains(background_scene->GetName()))
    return nullptr;
  if (impl_->foreground2background_.contains(background_scene))
    return nullptr;

  Scene* new_scene = new Scene(new_scene_name);
  impl_->scenes_.Insert(new_scene_name, new_scene);
  if (background_scene != nullptr)
    impl_->foreground2background_.insert(std::make_pair(new_scene,background_scene));
  if (!impl_->active_scene_)
    impl_->active_scene_ = new_scene;
  return new_scene;
}

bool Application::RemoveScene(Scene* scene) {
  if (!scene)
    return false;
  if (impl_->scenes_.Get(scene->GetName()) != scene)
    return false;

  bool is_background =
    (impl_->foreground2background_.find(scene) == impl_->foreground2background_.end());
  if (is_background) {
    // Remove foreground objects.
    std::list<Scene*> removable_fg_scenes;
    for (auto [f_scene, b_scene] : impl_->foreground2background_) {
      if (scene == b_scene)
        removable_fg_scenes.push_back(f_scene);
    }
    for (Scene* s : removable_fg_scenes)
      this->RemoveScene(s);
  } else {
    impl_->foreground2background_.erase(scene);
  }
  return impl_->scenes_.Remove(scene);
}

Scene* Application::GetScene(const std::string& name) {
  return dynamic_cast<Scene*>(impl_->scenes_.Get(name));
}

const Scene* Application::GetScene(const std::string& name) const {
  return dynamic_cast<Scene*>(impl_->scenes_.Get(name));
}

namespace {
class IteratorImpl_ForegroundScenes : public IteratorImpl<Scene*> {
 public:
  IteratorImpl_ForegroundScenes(
      const std::map<Scene*,Scene*>& f2b,
      Scene* background_scene,
      std::map<Scene*,Scene*>::const_iterator it = std::map<Scene*,Scene*>::const_iterator())
      : f2b_(f2b) {
    background_scene_ = background_scene;
    it_ = it;
    if (it_ == std::map<Scene*,Scene*>::const_iterator())
      it_ = f2b.begin();
    while (it_ != f2b_.end() && it_->second != background_scene_)
      ++it_;
  }

  virtual ~IteratorImpl_ForegroundScenes() {}

  void advance() override {
    ++it_;
    while (it_ != f2b_.end() && it_->second != background_scene_)
      ++it_;
  }

  Scene* current() const override { return it_->first; }

  IteratorImpl<Scene*>* clone() const override {
    return new IteratorImpl_ForegroundScenes{f2b_, background_scene_, it_};
  }

  IteratorImpl<Scene*>* last() const override {
    return new IteratorImpl_ForegroundScenes{f2b_, background_scene_, f2b_.end()};
  }

  bool end() const override { return it_ == f2b_.end(); }

  bool equals(const IteratorImpl<Scene*>& other) const override {
    auto it = dynamic_cast<const IteratorImpl_ForegroundScenes*>(&other);
    if (!it)
      return false;
    return it->background_scene_ == this->background_scene_ &&
           it->it_ == this->it_;
  }

 private:
   const std::map<Scene*,Scene*>& f2b_;
   Scene* background_scene_;
   std::map<Scene*,Scene*>::const_iterator it_;
};
}
Iterator<Scene*> Application::GetForegroundScenes(Scene* background_scene) {
  auto it = new IteratorImpl_ForegroundScenes(impl_->foreground2background_,
                                              background_scene);
  return Iterator<Scene*>(it);
}

Scene* Application::GetBackgroundScene(Scene* foreground_scene) {
  auto it = impl_->foreground2background_.find(foreground_scene);
  if (it == impl_->foreground2background_.end())
    return nullptr;
  return it->second;
}

FgImage* Application::AddFgImage(const std::string& image_filename) {
  FgImage fg(image_filename);
  if (fg.IsEmpty())
    return nullptr;
  auto fg_image = new FgImage(fg);
  impl_->fg_images_.Insert(fg_image);
  return fg_image;
}

FgImage* Application::AddFgImage(const unsigned char* image_data,
                                 size_t image_data_size) {
  FgImage fg(image_data, image_data_size);
  if (fg.IsEmpty())
    return nullptr;
  auto fg_image = new FgImage(fg);
  impl_->fg_images_.Insert(fg_image);
  return fg_image;
}

ScreenText* Application::AddScreenText(const std::string& text) {
  return nullptr;
}

bool Application::AddFgImage(Scene* scene, FgImage* fg_image,
                             const glm::ivec2& screen_position) {
  if (scene == nullptr || fg_image == nullptr ||
      screen_position.x < 0 || screen_position.y < 0)
    return false;

  scene->AddFgImage(fg_image, screen_position);
  return false;
}

bool Application::AddScreenText(Scene*, ScreenText*,
                                const glm::ivec2& screen_position) {
  return false;
}

bool Application::Remove(Scene*, const ScreenText*) {
  return false;
}

bool Application::Remove(Scene*, const FgImage*) {
  return false;
}

void Application::Sync() {
  impl_->scenes_.Synch();
  impl_->fg_images_.Synch();
  impl_->screen_texts_.Synch();
}
