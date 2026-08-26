#include "numgeom/debug_framework.h"

#include "numgeom/drawable.h"
#include "numgeom/scene.h"
#include "numgeom/sceneobject.h"

TriMesh::Ptr ConvertToTriMesh(const Scene* scene) {
  size_t n_verts = 0, n_cells = 0;
  for (SceneObject* so : scene->VisibleObjects()) {
    for (Drawable* d : so->VisibleDrawables()) {
      if (d->Type() != Drawable::PrimitiveType::Triangles)
        continue;
      n_verts += d->GetVertsCount();
      n_cells += d->GetCellsCount();
    }
  }
  if (n_verts == 0 || n_cells == 0)
    return TriMesh::Ptr();
  TriMesh::Ptr mesh = TriMesh::Create(n_verts, n_cells);
  size_t node_index = 0, cell_index = 0, node_offset = 0;
  for (SceneObject* so : scene->VisibleObjects()) {
    for (Drawable* d : so->VisibleDrawables()) {
      if (d->Type() != Drawable::PrimitiveType::Triangles)
        continue;
      auto d2 = Drawable2::Cast(d);
      for (glm::vec3 v : d->GetVertices()) {
        mesh->GetNode(node_index++) = v;
      }
      for (glm::u32vec3 tr : d2->GetTriangles()) {
        mesh->GetCell(cell_index++) = TriMesh::Cell(tr.x + node_offset,
                                                    tr.y + node_offset,
                                                    tr.z + node_offset);
      }
      node_offset += d->GetVertsCount();
    }
  }
  return mesh;
}
