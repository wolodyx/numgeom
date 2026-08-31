#ifndef NUMGEOM_FRAMEWORK_DRAWABLE_TORUS_H
#define NUMGEOM_FRAMEWORK_DRAWABLE_TORUS_H

#include "numgeom/drawable.h"

class FRAMEWORK_EXPORT Drawable2_Torus : public Drawable2 {
 public:
  Drawable2_Torus(SceneObject*);
  Drawable2_Torus(SceneObject*, const glm::vec3& center, const glm::vec3& normal,
                  float major_radius, float minor_radius, int segments,
                  int rings);
  virtual ~Drawable2_Torus();
  size_t GetVertsCount() const override;
  size_t GetCellsCount() const override;
  Iterator<glm::vec3> GetVertices() const override;
  AlignedBoundBox GetBoundBox() const override;
  Iterator<glm::u32vec3> GetTriangles() const override;
  Iterator<glm::vec3> GetNormals() const override;

  void SetSegments(int segments);
  void SetRings(int rings);
  void UpdateParams(const glm::vec3& center, const glm::vec3& normal,
                    float major_radius, float minor_radius);

 private:
  glm::vec3 center_;
  glm::vec3 normal_;
  float major_radius_;
  float minor_radius_;
  int segments_;
  int rings_;
};
#endif // !NUMGEOM_FRAMEWORK_DRAWABLE_TORUS_H
