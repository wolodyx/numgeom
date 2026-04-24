#ifndef NUMGEOM_OCC_DRAWABLE_POLYTRIANGULATION_H
#define NUMGEOM_OCC_DRAWABLE_POLYTRIANGULATION_H

#include "Standard_Handle.hxx"

#include "numgeom/drawable.h"

class Poly_Triangulation;

class Drawable2_PolyTriangulation : public Drawable2 {
 public:
  Drawable2_PolyTriangulation(SceneObject*, Handle(Poly_Triangulation));
  virtual ~Drawable2_PolyTriangulation();
  size_t GetVertsCount() const override;
  size_t GetCellsCount() const override;
  Iterator<glm::vec3> GetVertices() const override;
  Iterator<glm::u32vec3> GetTriangles() const override;
  Iterator<glm::vec3> GetNormals() const override;
 private:
  Handle(Poly_Triangulation) triangulation_;
};
#endif NUMGEOM_OCC_DRAWABLE_POLYTRIANGULATION_H
