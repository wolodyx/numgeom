#ifndef NUMGEOM_FRAMEWORK_DRAWABLEIMPLEMENTATIONS_H
#define NUMGEOM_FRAMEWORK_DRAWABLEIMPLEMENTATIONS_H

#include <functional>
#include <vector>

#include "numgeom/drawable.h"

class FRAMEWORK_EXPORT Drawable_CachedPoints : public virtual Drawable {
 public:
  Drawable_CachedPoints(SceneObject*);
  virtual ~Drawable_CachedPoints();
  size_t GetVertsCount() const override;
  Iterator<glm::vec3> GetVertices() const override;
  uint32_t AddPoint(const glm::vec3& point);
  void ClearPoints();
  void SetTransformFunction(std::function<glm::vec3(glm::vec3)>);

 private:
  std::vector<glm::vec3> points_;
  std::function<glm::vec3(glm::vec3)> transform_function_;
};

class FRAMEWORK_EXPORT Drawable0_Cached : public virtual Drawable_CachedPoints,
                                          public virtual Drawable0 {
 public:
  Drawable0_Cached(SceneObject*);
  virtual ~Drawable0_Cached();
  size_t GetCellsCount() const override;
  void ClearCells();
};

class FRAMEWORK_EXPORT Drawable1_Cached : public virtual Drawable_CachedPoints,
                                          public virtual Drawable1 {
 public:
  Drawable1_Cached(SceneObject*);
  virtual ~Drawable1_Cached();
  size_t GetCellsCount() const override;
  Iterator<glm::u32vec2> GetEdges() const override;
  uint32_t AddCell(const glm::u32vec2& cell);
  uint32_t AddCell(uint32_t, uint32_t);
  void ClearCells();

 private:
  std::vector<glm::u32vec2> cells_;
};

class FRAMEWORK_EXPORT Drawable2_Cached : public virtual Drawable_CachedPoints,
                                          public virtual Drawable2 {
 public:
  Drawable2_Cached(SceneObject*);
  virtual ~Drawable2_Cached();
  size_t GetCellsCount() const override;
  Iterator<glm::vec3> GetNormals() const override;
  Iterator<glm::u32vec3> GetTriangles() const override;
  uint32_t AddCell(const glm::u32vec3& cell);
  uint32_t AddCell(uint32_t, uint32_t, uint32_t);
  void ClearCells();

 private:
  std::vector<glm::u32vec3> cells_;
};
#endif  // !NUMGEOM_FRAMEWORK_DRAWABLEIMPLEMENTATIONS_H
