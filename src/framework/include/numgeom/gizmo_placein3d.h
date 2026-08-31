#ifndef NUMGEOM_FRAMEWORK_GIZMO_PLACEIN3D_H
#define NUMGEOM_FRAMEWORK_GIZMO_PLACEIN3D_H

#include "numgeom/gizmo.h"
#include "numgeom/orthobasis.h"

class Drawable1_Segment;
class Drawable2_Cone;
class Drawable2_Cylinder;
class Drawable2_Sphere;

class FRAMEWORK_EXPORT Gizmo_PlaceIn3d : public Gizmo {
 public:
  Gizmo_PlaceIn3d(Command*, const std::string& title,
                  const glm::vec3& position = glm::vec3(0.0f),
                  const OrthoBasis<float>& basis = OrthoBasis<float>());
  virtual ~Gizmo_PlaceIn3d();
  void Drag(Drawable*, const glm::vec3&) override;
  void Enter(Drawable*) override;
  void Exit(Drawable*) override;

  glm::vec3 GetPosition() const;
  OrthoBasis<float> GetBasis() const;
  glm::mat4x4 GetTransformMatrix() const;

  void SetPosition(const glm::vec3&);
  void SetBasis(const OrthoBasis<float>&);

 private:
  void UpdateSceneObject() override;

 private:
  glm::vec3 position_ = {0.0f, 0.0f, 0.0f};
  OrthoBasis<float> basis_;
  Drawable2_Sphere* sphere_ = nullptr;
  Drawable2_Cylinder* x_cylinder_ = nullptr;
  Drawable2_Cylinder* y_cylinder_ = nullptr;
  Drawable2_Cylinder* z_cylinder_ = nullptr;
  Drawable2_Cone* x_cone_ = nullptr;
  Drawable2_Cone* y_cone_ = nullptr;
  Drawable2_Cone* z_cone_ = nullptr;
  Drawable1_Segment* x_segment_ = nullptr;
  Drawable1_Segment* y_segment_ = nullptr;
  Drawable1_Segment* z_segment_ = nullptr;
};
#endif // !NUMGEOM_FRAMEWORK_GIZMO_PLACEIN3D_H
