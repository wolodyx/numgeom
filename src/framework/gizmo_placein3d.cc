#include "numgeom/gizmo_placein3d.h"

#include "numgeom/drawable_cone.h"
#include "numgeom/drawable_cylinder.h"
#include "numgeom/drawable_sphere.h"

#include "drawable_primitives.h"

Gizmo_PlaceIn3d::Gizmo_PlaceIn3d(Command* command, const std::string& title,
                                 const glm::vec3& position,
                                 const OrthoBasis<float>& basis)
    : Gizmo(command, title) {
  position_ = position;
  basis_ = basis;
  this->UpdateSceneObject();
}

Gizmo_PlaceIn3d::~Gizmo_PlaceIn3d() {
}

glm::vec3 Gizmo_PlaceIn3d::GetPosition() const { return position_; }

OrthoBasis<float> Gizmo_PlaceIn3d::GetBasis() const {
  return basis_;
}

glm::mat4x4 Gizmo_PlaceIn3d::GetTransformMatrix() const {
  return basis_.ToTransformMatrix(position_);
}

void Gizmo_PlaceIn3d::SetPosition(const glm::vec3& pos) {
  position_ = pos;
  this->UpdateSceneObject();
  this->NotifyAboutContentChanges();
}

void Gizmo_PlaceIn3d::SetBasis(const OrthoBasis<float>& basis) {
  basis_ = basis;
  this->UpdateSceneObject();
  this->NotifyAboutContentChanges();
}

void Gizmo_PlaceIn3d::UpdateSceneObject() {
  const size_t segments = 10;
  if (!sphere_) {
    sphere_ = this->AddDrawable<Drawable2_Sphere>();
    sphere_->SetSlicesAndStacks(segments, segments);
    sphere_->SetColor(1.0f, 1.0f, 0.0);
  }
  if (!x_cylinder_) {
    x_cylinder_ = this->AddDrawable<Drawable2_Cylinder>();
    x_cylinder_->SetSegments(segments);
    x_cylinder_->SetColor(1.0f, 0.0f, 0.0f);
  }
  if (!y_cylinder_) {
    y_cylinder_ = this->AddDrawable<Drawable2_Cylinder>();
    y_cylinder_->SetSegments(segments);
    y_cylinder_->SetColor(0.0f, 1.0f, 0.0f);
  }
  if (!z_cylinder_) {
    z_cylinder_ = this->AddDrawable<Drawable2_Cylinder>();
    z_cylinder_->SetSegments(segments);
    z_cylinder_->SetColor(0.0f, 0.0f, 1.0f);
  }
  if (!x_cone_) {
    x_cone_ = this->AddDrawable<Drawable2_Cone>();
    x_cone_->SetSegments(segments);
    x_cone_->SetColor(0.788f, 0.580f, 0.165f);
  }
  if (!y_cone_) {
    y_cone_ = this->AddDrawable<Drawable2_Cone>();
    y_cone_->SetSegments(segments);
    y_cone_->SetColor(0.788f, 0.580f, 0.165f);
  }
  if (!z_cone_) {
    z_cone_ = this->AddDrawable<Drawable2_Cone>();
    z_cone_->SetSegments(segments);
    z_cone_->SetColor(0.788f, 0.580f, 0.165f);
  }
  if (!x_segment_) {
    x_segment_ = this->AddDrawable<Drawable1_Segment>();
    x_segment_->SetColor(1.0f, 0.0f, 0.0f);
    x_segment_->SetVisibility(false);
  }
  if (!y_segment_) {
    y_segment_ = this->AddDrawable<Drawable1_Segment>();
    y_segment_->SetColor(0.0f, 1.0f, 0.0f);
    y_segment_->SetVisibility(false);
  }
  if (!z_segment_) {
    z_segment_ = this->AddDrawable<Drawable1_Segment>();
    z_segment_->SetColor(0.0f, 0.0f, 1.0f);
    z_segment_->SetVisibility(false);
  }
  const glm::vec3 orig = position_;
  const glm::vec3 x_axis = basis_.X();
  const glm::vec3 y_axis = basis_.Y();
  const glm::vec3 z_axis = basis_.Z();
  float ch_size = this->GetCharacteristicSize();
  const float sphere_radius = 0.2f * ch_size;
  const float axis_length = 1.0f * ch_size;
  const float axis_thickness = 0.05f * ch_size;
  const float cone_radius = 0.15f * ch_size;
  const float cone_height = 0.3f * ch_size;
  const glm::vec3 x_axis_end = orig + axis_length * x_axis;
  const glm::vec3 y_axis_end = orig + axis_length * y_axis;
  const glm::vec3 z_axis_end = orig + axis_length * z_axis;
  sphere_->UpdateParams(orig, sphere_radius);
  x_cylinder_->UpdateParams(orig, x_axis_end, axis_thickness);
  y_cylinder_->UpdateParams(orig, y_axis_end, axis_thickness);
  z_cylinder_->UpdateParams(orig, z_axis_end, axis_thickness);
  x_cone_->UpdateParams(x_axis_end, x_axis_end + cone_height * x_axis, cone_radius);
  y_cone_->UpdateParams(y_axis_end, y_axis_end + cone_height * y_axis, cone_radius);
  z_cone_->UpdateParams(z_axis_end, z_axis_end + cone_height * z_axis, cone_radius);
  x_segment_->SetVertices(orig - x_axis * 10.0f * ch_size,
                          x_axis_end + x_axis * 10.0f * ch_size);
  y_segment_->SetVertices(orig - y_axis * 10.0f * ch_size,
                          y_axis_end + y_axis * 10.0f * ch_size);
  z_segment_->SetVertices(orig - z_axis * 10.0f * ch_size,
                          z_axis_end + z_axis * 10.0f * ch_size);
  this->UpdateScene();
}

void Gizmo_PlaceIn3d::Drag(Drawable* drawable, const glm::vec3& dir) {
  if (drawable == sphere_) {
    position_ += dir;
  } else if (drawable == x_cylinder_) {
  } else if (drawable == y_cylinder_) {
  } else if (drawable == z_cylinder_) {
  } else if (drawable == x_cone_) {
    glm::vec3 axis(1.0f, 0.0f, 0.0f);
    position_ += (axis * glm::dot(axis,dir));
  } else if (drawable == y_cone_) {
    glm::vec3 axis(0.0f, 1.0f, 0.0f);
    position_ += (axis * glm::dot(axis,dir));
  } else if (drawable == z_cone_) {
    glm::vec3 axis(0.0f, 0.0f, 1.0f);
    position_ += (axis * glm::dot(axis,dir));
  } else {
    return;
  }
  this->UpdateSceneObject();
  this->NotifyAboutContentChanges();
}

void Gizmo_PlaceIn3d::Enter(Drawable* drawable) {
  if (drawable == x_cone_) {
    x_segment_->SetVisibility(true);
  } else if (drawable == y_cone_) {
    y_segment_->SetVisibility(true);
  } else if (drawable == z_cone_) {
    z_segment_->SetVisibility(true);
  } else {
    return;
  }
  this->UpdateScene();
}

void Gizmo_PlaceIn3d::Exit(Drawable* drawable) {
  if (drawable == x_cone_) {
    x_segment_->SetVisibility(false);
  } else if (drawable == y_cone_) {
    y_segment_->SetVisibility(false);
  } else if (drawable == z_cone_) {
    z_segment_->SetVisibility(false);
  } else {
    return;
  }
  this->UpdateScene();
}
