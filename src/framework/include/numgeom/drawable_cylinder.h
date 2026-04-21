#ifndef NUMGEOM_FRAMEWORK_DRAWABLE_CYLINDER_H
#define NUMGEOM_FRAMEWORK_DRAWABLE_CYLINDER_H

#include "numgeom/drawable.h"

class Drawable2_Cylinder : public Drawable2 {
 public:
  Drawable2_Cylinder(SceneObject*, const glm::vec3& bottom_center,
                     const glm::vec3& top_center, float radius, int segments);
  virtual ~Drawable2_Cylinder();
  size_t GetVertsCount() const override;
  size_t GetCellsCount() const override;
  Iterator<glm::vec3> GetVertices() const override;
  AlignedBoundBox GetBoundBox() const override;
  Iterator<glm::u32vec3> GetTriangles() const override;
  Iterator<glm::vec3> GetNormals() const override;
 private:
  glm::vec3 bottom_center_;
  glm::vec3 top_center_;
  float radius_;
  int segments_;
};
#endif // !NUMGEOM_FRAMEWORK_DRAWABLE_CYLINDER_H
