#ifndef NUMGEOM_OCC_SCENEOBJECT_POLYTRIANGULATION_H
#define NUMGEOM_OCC_SCENEOBJECT_POLYTRIANGULATION_H

#include "Standard_Handle.hxx"

#include "numgeom/sceneobject.h"

class Poly_Triangulation;

class SceneObject_PolyTriangulation : public SceneObject {
 public:
  SceneObject_PolyTriangulation(Scene*, Handle(Poly_Triangulation));
  virtual ~SceneObject_PolyTriangulation();
};
#endif // !NUMGEOM_OCC_SCENEOBJECT_POLYTRIANGULATION_H
