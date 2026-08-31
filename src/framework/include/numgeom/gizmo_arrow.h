#ifndef NUMGEOM_FRAMEWORK_GIZMO_ARROW_H
#define NUMGEOM_FRAMEWORK_GIZMO_ARROW_H

#include "numgeom/gizmo.h"

class Drawable2_Cone;
class Drawable2_Cylinder;
class Drawable2_Sphere;

class FRAMEWORK_EXPORT Gizmo_Arrow : public Gizmo {
 public:
  Gizmo_Arrow(Command*);
  virtual ~Gizmo_Arrow();
  void Drag(Drawable*, const glm::vec3&) override;
  void DoubleClick(MouseButton, Drawable*) override;

 private:
  void Update();

 private:
  glm::vec3 orig_;
  glm::vec3 dir_;
  float length_;
  Drawable2_Cone* head_;
  Drawable2_Cylinder* shaft_;
  Drawable2_Sphere* base_;
};
#endif // !NUMGEOM_FRAMEWORK_GIZMO_ARROW_H
