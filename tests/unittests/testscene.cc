#include "gtest/gtest.h"

#include "numgeom/scene.h"
#include "numgeom/sceneobject.h"
#include "numgeom/sceneobject_mesh.h"

TEST(Scene, IterateThroughVertex) {
  Scene scene;
  TriMesh::Ptr mesh;
  SceneObject* so_mesh = scene.AddObject<SceneObject_Mesh>(mesh);
  ASSERT_TRUE(so_mesh != nullptr);
}
