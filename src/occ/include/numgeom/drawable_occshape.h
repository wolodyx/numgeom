#ifndef NUMGEOM_OCC_OCCSHAPE_H
#define NUMGEOM_OCC_OCCSHAPE_H

#include "TopoDS_Shape.hxx"
#include "gp_Trsf.hxx"

#include "numgeom/drawable.h"
#include "numgeom/trimesh.h"

class Poly_Triangulation;

class Drawable2_OccShape : public Drawable2 {
 public:
   Drawable2_OccShape(SceneObject*, const TopoDS_Shape&, const gp_Trsf&);
   virtual ~Drawable2_OccShape();
   virtual size_t GetVertsCount() const;
   virtual size_t GetCellsCount() const;
   virtual Iterator<glm::vec3> GetVertices() const;
   virtual Iterator<glm::u32vec3> GetTriangles() const;
   virtual Iterator<glm::vec3> GetNormals() const;

 private:
   TopoDS_Shape shape_;
   gp_Trsf trsf_;
   std::list<std::tuple<Handle(Poly_Triangulation),gp_Trsf,bool>> triangulations_;
   size_t verts_num_;
   size_t cells_num_;
};
#endif // !NUMGEOM_OCC_OCCSHAPE_H
