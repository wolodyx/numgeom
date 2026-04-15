#ifndef NUMGEOM_FRAMEWORK_SCENEOBJECT_MESH_H
#define NUMGEOM_FRAMEWORK_SCENEOBJECT_MESH_H

#include "numgeom/sceneobject.h"
#include "numgeom/trimesh.h"

class SceneObject_Mesh : public SceneObject {
 public:
  SceneObject_Mesh(Scene*, CTriMesh::Ptr);
  virtual ~SceneObject_Mesh();

 private:
};
#endif // !NUMGEOM_FRAMEWORK_SCENEOBJECT_MESH_H
