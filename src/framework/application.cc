#include "numgeom/application.h"

#include <format>
#include <iostream>
#include <map>

#include "numgeom/alignedboundbox.h"
#include "numgeom/appcontext.h"
#include "numgeom/command.h"
#include "numgeom/drawable.h"
#include "numgeom/fgtext.h"
#include "numgeom/guimediator.h"
#include "numgeom/iteratorimpl.hpp"
#include "numgeom/scene.h"
#include "numgeom/sceneobject_mesh.h"
#include "numgeom/trimesh.h"
#include "numgeom/vkscenerenderer.h"
#include "numgeom/workbench.h"

#include "fgimage.h"
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
  AppContext context;
  GuiMediator* gui_mediator = nullptr;
  TrackedObjectList scenes_;
  TrackedObjectList fg_objects_;
  VkSceneRenderer* renderer_ = nullptr;
  WorkplaneAttributes workplane_attributes_;
  Command* command_ = nullptr;
  std::map<std::string, Workbench*> workbenches_;
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

static Application* s_instance = nullptr;

Application::Application(int argc, char* argv[]) {
  impl_ = new ApplicationImpl();
  impl_->renderer_ = new VkSceneRenderer(this);
  if (s_instance)
    std::abort();
  s_instance = this;
}

Application* Application::Get() {
  return s_instance;
}

Application::~Application() { delete impl_; }

VkSceneRenderer* Application::GetRenderer() { return impl_->renderer_; }

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

void Application::Sync() {
  impl_->scenes_.Synch();
  impl_->fg_objects_.Synch();
}

const WorkplaneAttributes* Application::GetWorkplaneAttributes() const {
  return &impl_->workplane_attributes_;
}

bool Application::Add(Workbench* workbench) {
  if (!workbench)
    return false;
  std::string name = workbench->GetName();
  if (name.empty())
    return false;
  if (impl_->workbenches_.find(name) != impl_->workbenches_.end())
    return false;
  impl_->workbenches_.insert(std::make_pair(workbench->GetName(),workbench));
  return true;
}

bool Application::Remove(Workbench* workbench) {
  auto it = impl_->workbenches_.find(workbench->GetName());
  if (it == impl_->workbenches_.end())
    return false;
  impl_->workbenches_.erase(it);
  delete workbench;
  return true;
}

Workbench* Application::GetWorkbench(const std::string& name) const {
  auto it = impl_->workbenches_.find(name);
  if (it == impl_->workbenches_.end())
    return nullptr;
  return it->second;
}

Iterator<Workbench*> Application::GetWorkbenches() const {
  auto it_impl = new IteratorImpl_StdMapValue<std::string, Workbench*>(
      impl_->workbenches_.begin(), impl_->workbenches_.end());
  return Iterator<Workbench*>(it_impl);
}

namespace {
std::pair<std::string,std::string> SplitByFirstDot(const std::string& str) {
  auto pos = str.find('.');
  if (pos == std::string::npos)
    return std::make_pair(str, "");
  return std::make_pair(str.substr(0, pos), str.substr(pos + 1));
}
}

bool Application::SetupCommand(const std::string& command_id) {
  // Уничтожаем предыдущую команду.
  if (impl_->command_) {
    if (impl_->gui_mediator)
      impl_->gui_mediator->DisableCommandMode();
    impl_->command_->Cleanup();
    delete impl_->command_;
    impl_->command_ = nullptr;
  }

  // Создаем новую команду через модуль.
  std::string workbench_type_id, command_name;
  std::tie(workbench_type_id, command_name) = SplitByFirstDot(command_id);
  Workbench* wb = this->GetWorkbench(workbench_type_id);
  if (!wb) {
    std::cerr << std::format("Failed to find workbench {}", workbench_type_id)
              << std::endl;
    return false;
  }
  impl_->command_ = wb->CreateCommand(command_name);
  if (!impl_->command_) {
    std::cerr << std::format("Failed to create command {} from module {}.",
                             command_name, workbench_type_id)
              << std::endl;
    return false;
  }

  impl_->command_->Setup();
  impl_->command_->Recover();
  if (impl_->gui_mediator)
    impl_->gui_mediator->EnableCommandMode();
  return true;
}

Outcome Application::ExecuteCommand() {
  if (!impl_->command_)
    return Outcome::Error("No command to execute");
  auto outcome = impl_->command_->Execute();
  if (impl_->gui_mediator)
    impl_->gui_mediator->ShowMessage(outcome);
  impl_->command_->Recover();
  impl_->command_->GetWorkbench()->UpdateScreen();
  return outcome;
}

void Application::CancelCommand() {
  if (!impl_->command_)
    return;
  impl_->command_->Cleanup();
  delete impl_->command_;
  impl_->command_ = nullptr;
  if (impl_->gui_mediator)
    impl_->gui_mediator->DisableCommandMode();
}

Command* Application::GetCommand() const {
  return impl_->command_;
}

AppContext* Application::GetContext() {
  return &impl_->context;
}

void Application::SetGuiMediator(GuiMediator* gui_mediator) {
  impl_->gui_mediator = gui_mediator;
}
