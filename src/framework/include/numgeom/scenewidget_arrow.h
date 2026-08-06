#ifndef NUMGEOM_FRAMEWORK_SCENEWIDGET_ARROW_H
#define NUMGEOM_FRAMEWORK_SCENEWIDGET_ARROW_H

#include "numgeom/framework_export.h"
#include "numgeom/scenewidget.h"

class Drawable2_Cone;
class Drawable2_Cylinder;
class Drawable2_Sphere;

class SceneWidget_Arrow : public SceneWidget {
 public:
  SceneWidget_Arrow(Scene*);
  virtual ~SceneWidget_Arrow();
  void Drag(Drawable*, const glm::vec3&) override;

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
#endif // !NUMGEOM_FRAMEWORK_SCENEWIDGET_ARROW_H
