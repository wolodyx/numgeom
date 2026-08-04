#include "numgeom/scenewidget_arrow.h"

#include "numgeom/drawable_cone.h"
#include "numgeom/drawable_cylinder.h"
#include "numgeom/drawable_sphere.h"

SceneWidget_Arrow::SceneWidget_Arrow(Scene* scene)
    : SceneWidget(scene) {
  const glm::vec3 orig(0.0f,0.0f,0.0f);
  const glm::vec3 x_axis = glm::vec3(1.f,0.f,0.f);
  const float sphere_radius = 0.2f;
  const float axis_length = 1.0f;
  const float axis_thickness = 0.05f;
  const float cone_radius = 0.15f;
  const float cone_height = 0.3f;
  const size_t segments = 10;

  const glm::vec3 shaft_end = orig + axis_length * x_axis;
  base_ = this->AddDrawable<Drawable2_Sphere>(orig, sphere_radius, segments, segments);
  shaft_ = this->AddDrawable<Drawable2_Cylinder>(orig, shaft_end, axis_thickness, segments);
  head_ = this->AddDrawable<Drawable2_Cone>(shaft_end, shaft_end + cone_height * x_axis, cone_radius, segments);

  head_->SetColor(0.788f, 0.580f, 0.165f);
  shaft_->SetColor(1.0f,0.0f,0.0f);
  base_->SetColor(1.0f, 1.0f, 0.0);
}

SceneWidget_Arrow::~SceneWidget_Arrow() {
}
