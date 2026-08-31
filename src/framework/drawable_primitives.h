#ifndef NUMGEOM_FRAMEWORK_DRAWABLE_PRIMITIVES_H
#define NUMGEOM_FRAMEWORK_DRAWABLE_PRIMITIVES_H

#include "numgeom/drawable.h"

class FRAMEWORK_EXPORT Drawable1_Segment : public virtual Drawable1 {
 public:
  Drawable1_Segment(SceneObject*);
  Drawable1_Segment(SceneObject*, const glm::vec3& start, const glm::vec3& end);
  virtual ~Drawable1_Segment();
  Iterator<glm::u32vec2> GetEdges() const override;
  size_t GetVertsCount() const override;
  size_t GetCellsCount() const override;
  Iterator<glm::vec3> GetVertices() const override;
  void SetVertices(const glm::vec3& start, const glm::vec3& end);

 private:
  glm::vec3 start_;
  glm::vec3 end_;
};
#endif // !NUMGEOM_FRAMEWORK_DRAWABLE_PRIMITIVES_H
