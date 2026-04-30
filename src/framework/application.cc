#include "numgeom/application.h"

#include <format>
#include <iostream>

#include "numgeom/alignedboundbox.h"
#include "numgeom/gpumanager.h"
#include "numgeom/scene.h"
#include "numgeom/sceneobject_mesh.h"
#include "numgeom/scenewidget_axisindicator.h"
#include "numgeom/trimesh.h"

#include "camera.h"

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

struct Application::Impl {
  Camera camera;

  const float fovy = glm::radians(45.0f);

  Scene scene;

  GpuManager* gpuManager;
};

Application::Application(int argc, char* argv[]) {
  m_pimpl = new Impl();
  m_pimpl->gpuManager = new GpuManager(this);
  m_pimpl->camera.SetBoundBoxFunction(
    [this](){ return this->m_pimpl->scene.GetBoundBox(); });
}

Application::~Application() { delete m_pimpl; }

void Application::FitScene() {
  AlignedBoundBox box = m_pimpl->scene.GetBoundBox();
  box.Scale(1.3);
  m_pimpl->camera.FitBox(box);
  this->Update();
}

void Application::ZoomCamera(float k) {
  m_pimpl->camera.Zoom(k);
  this->Update();
}

glm::vec3 Application::CameraPosition() const {
  return m_pimpl->camera.GetPosition();
}

void Application::TranslateCamera(int x, int y, int dx, int dy) {
  if (dx == 0 && dy == 0) return;
  m_pimpl->camera.Translate(glm::ivec2{dx,dy});
  this->Update();
}

void Application::RotateCamera(int x, int y, int dx, int dy) {
  if (dx == 0 && dy == 0) return;
  AlignedBoundBox box = m_pimpl->scene.GetBoundBox();
  if (box.IsEmpty())
    return;
  m_pimpl->camera.SetPivotPoint(box.GetCenter());
  glm::vec2 screen_offset(static_cast<float>(dx), static_cast<float>(dy));
  m_pimpl->camera.RotateAroundPivot(screen_offset);
  this->Update();
}

glm::mat4 Application::GetViewMatrix() const {
  auto x = m_pimpl->camera.GetViewMatrix();
  return x;
}

glm::mat4 Application::GetProjectionMatrix() const {
  return m_pimpl->camera.GetProjectionMatrix(m_pimpl->scene.GetBoundBox());
}

void Application::Update() { m_pimpl->gpuManager->update(); }

GpuManager* Application::GetGpuManager() { return m_pimpl->gpuManager; }

const Scene& Application::GetScene() const { return m_pimpl->scene; }

Scene& Application::GetScene() { return m_pimpl->scene; }

void Application::ClearScene() {
  m_pimpl->scene.Clear();
  this->Update();
}

void Application::SetViewportSizeFunction(std::function<std::tuple<uint32_t,uint32_t>()> func) {
  m_pimpl->camera.SetViewportSizeFunction(func);
}
