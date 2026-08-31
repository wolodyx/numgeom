#include "numgeom/workbench.h"

#include <filesystem>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include "numgeom/application.h"
#include "numgeom/drawable.h"
#include "numgeom/iteratorimpl.hpp"
#include "numgeom/ray.h"
#include "numgeom/scene.h"
#include "numgeom/sceneobject.h"
#include "numgeom/userinputcontroller.h"
#include "numgeom/vkscenerenderer.h"

#include "intersection.h"

static std::map<std::string, std::function<Workbench*()>>& GetCreators() {
  static std::map<std::string, std::function<Workbench*()>> g_creators;
  return g_creators;
}

void Workbench::Register(const std::string& workbench_type_name,
                         std::function<Workbench*()> create_function) {
  if (workbench_type_name.empty())
    return;
  auto& creators_ = GetCreators();
  auto it = creators_.find(workbench_type_name);
  if (creators_.find(workbench_type_name) != creators_.end())
    return;
  creators_.insert(std::make_pair(workbench_type_name,create_function));
}

Iterator<std::string> Workbench::GetWorkbenchTypes() {
  auto& creators_ = GetCreators();
  auto it_impl =
      new IteratorImpl_StdMapKey<std::string,std::function<Workbench*()>>(
          creators_.begin(),creators_.end());
  return Iterator<std::string>(it_impl);
}

Workbench* Workbench::Create(const std::string& type_name) {
  auto& creators_ = GetCreators();
  auto it = creators_.find(type_name);
  if (it == creators_.end())
    return nullptr;
  return (it->second)();
}

bool Workbench::LoadModule(const std::filesystem::path& library_path) {
  if (library_path.empty())
    return false;
#if defined(_WIN32)
  HMODULE module = ::LoadLibraryW(library_path.c_str());
  return module != nullptr;
#else
  void* handle = ::dlopen(library_path.c_str(), RTLD_NOW | RTLD_GLOBAL);
  return handle != nullptr;
#endif
}

static std::map<
    std::string,
    std::function<Command*(Workbench*)>> g_command_registry;

bool Workbench::RegisterCommand(
    const std::string& type_id,
    const std::string& cmd_id,
    std::function<Command*(Workbench*)> create_function) {
  auto key = type_id + '.' + cmd_id;
  auto ib = g_command_registry.insert(std::make_pair(key,create_function));
  return ib.second;
}

Iterator<std::string> Workbench::GetCommandIds(const std::string& workbench_type_id) {
  IteratorImpl<std::string>* it_impl_extract_key =
      new IteratorImpl_StdMapKey<
          std::string,
          std::function<Command*(Workbench*)>>(g_command_registry);
  //IteratorImpl<std::string>* it_impl_transform_key =
  //    new IteratorImpl_Transform<std::string,std::string>(
  //        it_impl_extract_key,
  //        [](const std::string& v){
  //          auto pos = v.find('.');
  //          if (pos == std::string::npos)
  //            return v;
  //          return v.substr(pos + 1);
  //        });
  return Iterator<std::string>(it_impl_extract_key);
}

class WorkbenchImpl {
 public:
  Scene* scene = nullptr;
  UserInputController* uic = nullptr;
};

Workbench::Workbench() {
  impl_ = new WorkbenchImpl{
    .scene = new Scene{},
    .uic = new UserInputController(this),
  };
}

Workbench::~Workbench() {
  delete impl_->scene;
  delete impl_->uic;
  delete impl_;
}

Command* Workbench::CreateCommand(const std::string& cmd_id) {
  auto key = this->GetName() + '.' + cmd_id;
  auto it = g_command_registry.find(key);
  if (it == g_command_registry.end())
    return nullptr;
  if (!it->second)
    return nullptr;
  return (it->second)(this);
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

Drawable* Workbench::Pick(int x, int y, glm::vec3* picked_point) const {
  Application* app = Application::Get();
  VkSceneRenderer* renderer = app->GetRenderer();
  const Scene* scene = this->GetScene();
  uint32_t id = renderer->GetObjectId(scene, x, y);
  if (id == 0) return nullptr;
  Drawable* picked_item = nullptr;
  for (SceneObject* o : scene->Objects()) {
    for (Drawable* d : o->Drawables()) {
      if (d->GetId() == id) {
        picked_item = d;
        break;
      }
    }
    if (picked_item) break;
  }
  if (!picked_item || !picked_item->IsPickable())
    return nullptr;
  if (picked_point) {
    Ray ray = ScreenPosToRay(scene, glm::ivec2(x, y));
    *picked_point = IntersectRayWithDrawable(ray, picked_item);
  }
  return picked_item;
}

Scene* Workbench::GetScene() {
  return impl_->scene;
}

const Scene* Workbench::GetScene() const {
  return impl_->scene;
}

void Workbench::UpdateScreen(bool force) {
  Scene* scene = this->GetScene();
  if (scene) {
    Application* app = Application::Get();
    app->GetRenderer()->Update(scene, force);
  }
}

void Workbench::Remove() {
  impl_->scene->MarkForDeletion();
}

UserInputController* Workbench::GetUserInputController() {
  return impl_->uic;
}

Outcome Workbench::LoadFromFile(const std::filesystem::path& filepath,
                                const glm::mat4x4& transform) {
  return Outcome::Error("Not implemented");
}

void Workbench::Clear() {}

void Workbench::LoadWelcomeModel() {}
