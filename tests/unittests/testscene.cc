#include "gtest/gtest.h"

#include "numgeom/drawable.h"
#include "numgeom/loadfromvtk.h"
#include "numgeom/scene.h"
#include "numgeom/sceneobject.h"
#include "numgeom/sceneobject_mesh.h"

#include "utilities.h"

TEST(Scene, IterateThroughVertex) {
  Scene scene;
  TriMesh::Ptr mesh = LoadTriMeshFromVtk(TestData("polydata-cube.vtk"));
  SceneObject* so_mesh = scene.AddObject<SceneObject_Mesh>(mesh);
  ASSERT_TRUE(so_mesh != nullptr);
  float s = 0.0f;
  for (Drawable* d : so_mesh->Drawables()) {
    for (glm::vec3 v : d->GetVertices()) {
      s += v.x += v.y += v.z;
    }
    if (d->Type() == Drawable::PrimitiveType::Triangles) {
      Drawable2* d2 = Drawable2::Cast(d);
      for(glm::vec3 n : d2->GetNormals()) {
        s += n.x += n.y += n.z;
      }
    }
  }
}
