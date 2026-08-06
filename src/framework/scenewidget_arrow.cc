#include "numgeom/scenewidget_arrow.h"

#include "numgeom/drawable_cone.h"
#include "numgeom/drawable_cylinder.h"
#include "numgeom/drawable_sphere.h"

SceneWidget_Arrow::SceneWidget_Arrow(Scene* scene)
    : SceneWidget(scene) {

  orig_ = glm::vec3(0.0f, 0.0f, 0.0f);
  dir_ = glm::vec3(1.0f, 0.0f, 0.0f);
  length_ = 1.0f;

  const size_t segments = 10;
  base_ = this->AddDrawable<Drawable2_Sphere>();
  shaft_ = this->AddDrawable<Drawable2_Cylinder>();
  head_ = this->AddDrawable<Drawable2_Cone>();
  base_->SetSlicesAndStacks(segments, segments);
  shaft_->SetSegments(segments);
  head_->SetSegments(segments);
  head_->SetColor(0.788f, 0.580f, 0.165f);
  shaft_->SetColor(1.0f,0.0f,0.0f);
  base_->SetColor(1.0f, 1.0f, 0.0);

  this->Update();
}

SceneWidget_Arrow::~SceneWidget_Arrow() {
}

void SceneWidget_Arrow::Update() {
  const float sphere_radius = 0.2f;
  const float axis_thickness = 0.05f;
  const float cone_radius = 0.15f;
  const float cone_height = 0.3f;

  const glm::vec3 shaft_end = orig_ + length_ * dir_;
  base_->UpdateParams(orig_, sphere_radius);
  shaft_->UpdateParams(orig_, shaft_end, axis_thickness);
  head_->UpdateParams(shaft_end, shaft_end + cone_height * dir_, cone_radius);
}

void SceneWidget_Arrow::Drag(Drawable* item, const glm::vec3& dragging_dir) {
  if (item == base_) {
    orig_ += dragging_dir;
  } else if (item == shaft_) {
    return;
  } else if (item == head_) {
    length_ += glm::dot(dir_, dragging_dir);
  }
  this->Update();
}
