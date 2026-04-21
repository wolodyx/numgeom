#include "numgeom/scenewidget_axisindicator.h"

#include "numgeom/drawable_cone.h"
#include "numgeom/drawable_cylinder.h"
#include "numgeom/drawable_sphere.h"

SceneWidget_AxisIndicator::SceneWidget_AxisIndicator(Scene* scene)
    : SceneWidget(scene) {
  const glm::vec3 orig(0.0f,0.0f,0.0f);
  const glm::vec3 x_axis = glm::vec3(1.f,0.f,0.f);
  const glm::vec3 y_axis = glm::vec3(0.f,1.f,0.f);
  const glm::vec3 z_axis = glm::vec3(0.f,0.f,1.f);
  const float sphere_radius = 0.2f;
  const float axis_length = 1.0f;
  const float axis_thickness = 0.05f;
  const float cone_radius = 0.15f;
  const float cone_height = 0.3f;
  const size_t segments = 10;

  const glm::vec3 x_axis_end = orig + axis_length * x_axis;
  const glm::vec3 y_axis_end = orig + axis_length * y_axis;
  const glm::vec3 z_axis_end = orig + axis_length * z_axis;
  sphere_ = this->AddDrawable<Drawable2_Sphere>(orig, sphere_radius, segments, segments);
  x_cylinder_ = this->AddDrawable<Drawable2_Cylinder>(orig, x_axis_end, axis_thickness, segments);
  y_cylinder_ = this->AddDrawable<Drawable2_Cylinder>(orig, y_axis_end, axis_thickness, segments);
  z_cylinder_ = this->AddDrawable<Drawable2_Cylinder>(orig, z_axis_end, axis_thickness, segments);
  x_cone_ = this->AddDrawable<Drawable2_Cone>(x_axis_end, x_axis_end + cone_height * x_axis, cone_radius, segments);
  y_cone_ = this->AddDrawable<Drawable2_Cone>(y_axis_end, y_axis_end + cone_height * y_axis, cone_radius, segments);
  z_cone_ = this->AddDrawable<Drawable2_Cone>(z_axis_end, z_axis_end + cone_height * z_axis, cone_radius, segments);

  x_cylinder_->SetColor(1.0f,0.0f,0.0f);
  y_cylinder_->SetColor(0.0f,1.0f,0.0f);
  z_cylinder_->SetColor(0.0f,0.0f,1.0f);
  sphere_->SetColor(1.0f, 1.0f, 0.0);
  x_cone_->SetColor(0.788, 0.580, 0.165);
  y_cone_->SetColor(0.788, 0.580, 0.165);
  z_cone_->SetColor(0.788, 0.580, 0.165);
}

SceneWidget_AxisIndicator::~SceneWidget_AxisIndicator() {
}
