#include "numgeom/application.h"

#include <format>
#include <iostream>

#include "numgeom/alignedboundbox.h"
#include "numgeom/gpumanager.h"
#include "numgeom/scene.h"
#include "numgeom/sceneobject_mesh.h"
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

  // void updateScene(CTriMesh::Ptr newScene) {
  //   if (scene == newScene) return;
  //   scene = newScene;
  //   auto box = ComputeBoundBox(scene);
  //   camera.fitBox(box);
  // }
};

Application::Application(int argc, char* argv[]) {
  m_pimpl = new Impl();
  m_pimpl->gpuManager = new GpuManager(this);
}

Application::~Application() { delete m_pimpl; }

void Application::fitScene() {
  m_pimpl->camera.fitBox(m_pimpl->scene.GetBoundBox());
  this->update();
}

void Application::zoomCamera(float k) {
  m_pimpl->camera.zoom(k);
  this->update();
}

glm::vec3 Application::CameraPosition() const {
  return m_pimpl->camera.m_position;
}

void Application::translateCamera(int x, int y, int dx, int dy) {
  if (dx == 0 && dy == 0) return;
  glm::vec2 screenOffset(static_cast<float>(dx), static_cast<float>(dy));
  m_pimpl->camera.translate(screenOffset);
  this->update();
}

void Application::rotateCamera(int x, int y, int dx, int dy) {
  if (dx == 0 && dy == 0) return;
  glm::vec2 screenOffset(static_cast<float>(dx), static_cast<float>(dy));
  glm::vec3 pivotPoint = m_pimpl->scene.GetBoundBox().GetCenter();
  m_pimpl->camera.rotateAroundPivot(pivotPoint, screenOffset);
  this->update();
}

glm::mat4 Application::getViewMatrix() const {
  auto x = m_pimpl->camera.viewMatrix();
  // std::cout << std::endl;
  // for(int i = 0; i < 4; ++i) {
  //     for(int j = 0; j < 4; ++j) {
  //         std::cout << x[i][j] << ' ';
  //     }
  //     std::cout << std::endl;
  // }
  return x;
}

glm::mat4 Application::getProjectionMatrix() const {
  return m_pimpl->camera.projectionMatrix(m_pimpl->scene.GetBoundBox());
}

void Application::update() { m_pimpl->gpuManager->update(); }

GpuManager* Application::gpuManager() { return m_pimpl->gpuManager; }

const Scene& Application::scene() const { return m_pimpl->scene; }
Scene& Application::scene() { return m_pimpl->scene; }

void Application::add(CTriMesh::Ptr mesh) {
  m_pimpl->scene.AddObject<SceneObject_Mesh>(mesh);
  this->update();
}

void Application::clearScene() {
  m_pimpl->scene.Clear();
  this->update();
}

void Application::set_aspect_function(std::function<float()> func) {
  m_pimpl->camera.setAspectFunction(func);
}
