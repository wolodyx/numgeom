#include "numgeom/gizmo_arrow.h"

#include <cmath>

#include "numgeom/drawable_cone.h"
#include "numgeom/drawable_cylinder.h"
#include "numgeom/drawable_sphere.h"

Gizmo_Arrow::Gizmo_Arrow(Command* command)
    : Gizmo(command, "Arrow") {

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

Gizmo_Arrow::~Gizmo_Arrow() {
}

void Gizmo_Arrow::Update() {
  const float sphere_radius = 0.2f;
  const float axis_thickness = 0.05f;
  const float cone_radius = 0.15f;
  const float cone_height = 0.3f;

  const glm::vec3 shaft_end = orig_ + length_ * dir_;
  base_->UpdateParams(orig_, sphere_radius);
  shaft_->UpdateParams(orig_, shaft_end, axis_thickness);
  head_->UpdateParams(shaft_end, shaft_end + cone_height * dir_, cone_radius);
}

void Gizmo_Arrow::Drag(Drawable* item, const glm::vec3& dragging_dir) {
  if (item == base_) {
    orig_ += dragging_dir;
  } else if (item == shaft_) {
    length_ += glm::dot(dir_, dragging_dir);
  } else if (item == head_) {
    const glm::vec3 tip = orig_ + length_ * dir_ + dragging_dir;
    const glm::vec3 new_dir = tip - orig_;
    const float new_length = glm::length(new_dir);
    if (new_length > 1e-6f) {
      dir_ = new_dir / new_length;
    }
  }
  this->Update();
}

void Gizmo_Arrow::DoubleClick(MouseButton button, Drawable* item) {
  if (button != MouseButton::Left)
    return;
  if (item == base_) {
    orig_ = glm::round(orig_);
  } else if (item == shaft_) {
    const float ax = std::fabs(dir_.x);
    const float ay = std::fabs(dir_.y);
    const float az = std::fabs(dir_.z);
    glm::vec3 axis = glm::vec3(0.0f, 0.0f, 0.0f);
    if (ax >= ay && ax >= az) {
      axis = glm::vec3(glm::sign(dir_.x), 0.0f, 0.0f);
    } else if (ay >= az) {
      axis = glm::vec3(0.0f, glm::sign(dir_.y), 0.0f);
    } else {
      axis = glm::vec3(0.0f, 0.0f, glm::sign(dir_.z));
    }
    dir_ = axis;
  } else if (item == head_) {
    float snapped = std::round(length_);
    if (snapped == 0.0f) {
      snapped = (length_ < 0.0f) ? -1.0f : 1.0f;
    }
    length_ = snapped;
  }
  this->Update();
}
