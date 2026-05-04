#include "numgeom/application.h"

#include <format>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "numgeom/alignedboundbox.h"
#include "numgeom/gpumanager.h"
#include "numgeom/sceneobject_mesh.h"
#include "numgeom/scenewidget_axisindicator.h"
#include "numgeom/trimesh.h"

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
  m_pimpl = new State();
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

void Application::OrientCamera(const OrthoBasis<float>& ortho_basis) {
  m_pimpl->camera.Orient(ortho_basis);
  this->FitScene();
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

void Application::SetLogo(const std::string& image_filename,
                          const glm::ivec2& screen_position) {
  unsigned char* pixels = stbi_load(image_filename.c_str(),
                                    &m_pimpl->logo.width, &m_pimpl->logo.height,
                                    &m_pimpl->logo.channels, 4);
  if (!pixels)
    return;
  int pixels_size = m_pimpl->logo.width * m_pimpl->logo.height * 4;
  m_pimpl->logo.pixels.assign(pixels, pixels + pixels_size);
  stbi_image_free(pixels);
}

void Application::SetLogo(const unsigned char* image_data,
                          size_t image_data_size,
                          const glm::ivec2& screen_position) {
  m_pimpl->logo.position = screen_position;
  int width, height, channels;
  unsigned char* pixels = stbi_load_from_memory(
      image_data, static_cast<int>(image_data_size), &width, &height, &channels,
      4);
  if (!pixels)
    return;
  int pixels_size = width * height * 4;
  m_pimpl->logo.pixels.assign(pixels, pixels + pixels_size);
  m_pimpl->logo.width = width;
  m_pimpl->logo.height = height;
  m_pimpl->logo.channels = channels;
  stbi_image_free(pixels);
}

Application::Inner* Application::GetInnerInterface() {
  if (m_pimpl->inner_interface_ == nullptr)
    m_pimpl->inner_interface_ = new Inner(m_pimpl);
  return m_pimpl->inner_interface_;
}

const Application::Inner* Application::GetInnerInterface() const {
  if (m_pimpl->inner_interface_ == nullptr)
    m_pimpl->inner_interface_ = new Inner(m_pimpl);
  return m_pimpl->inner_interface_;
}
