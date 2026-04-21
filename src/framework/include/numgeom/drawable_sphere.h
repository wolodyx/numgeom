#ifndef NUMGEOM_FRAMEWORK_DRAWABLE_SPHERE_H
#define NUMGEOM_FRAMEWORK_DRAWABLE_SPHERE_H

#include "numgeom/drawable.h"

class Drawable2_Sphere : public Drawable2 {
 public:
  Drawable2_Sphere(SceneObject*, const glm::vec3& center, float radius,
                   int slices_num, int stacks_num);
  virtual ~Drawable2_Sphere();
  size_t GetVertsCount() const override;
  size_t GetCellsCount() const override;
  Iterator<glm::vec3> GetVertices() const override;
  AlignedBoundBox GetBoundBox() const override;
  Iterator<glm::u32vec3> GetTriangles() const override;
  Iterator<glm::vec3> GetNormals() const override;
private:
  int slices_num_, stacks_num_;
  glm::vec3 center_;
  float radius_;
};
#endif // !NUMGEOM_FRAMEWORK_DRAWABLE_SPHERE_H
