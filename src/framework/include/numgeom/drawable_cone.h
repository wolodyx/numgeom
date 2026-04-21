#ifndef NUMGEOM_FRAMEWORK_DRAWABLE_CONE_H
#define NUMGEOM_FRAMEWORK_DRAWABLE_CONE_H

#include "numgeom/drawable.h"

class Drawable2_Cone : public Drawable2 {
 public:
  Drawable2_Cone(SceneObject*, const glm::vec3& base_center,
                 const glm::vec3& apex, float radius, int segments);
  virtual ~Drawable2_Cone();
  size_t GetVertsCount() const override;
  size_t GetCellsCount() const override;
  Iterator<glm::vec3> GetVertices() const override;
  AlignedBoundBox GetBoundBox() const override;
  Iterator<glm::u32vec3> GetTriangles() const override;
  Iterator<glm::vec3> GetNormals() const override;
 private:
  glm::vec3 base_center_;
  glm::vec3 apex_;
  float radius_;
  int segments_;
};
#endif // !NUMGEOM_FRAMEWORK_DRAWABLE_CONE_H
