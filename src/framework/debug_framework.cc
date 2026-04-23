#include "numgeom/debug_framework.h"

#include "numgeom/drawable.h"
#include "numgeom/scene.h"
#include "numgeom/sceneobject.h"

#include "sceneiterators.h"

TriMesh::Ptr ConvertToTriMesh(const Scene& scene) {
  size_t n_verts = 0, n_cells = 0;
  GetElementsCount(scene, n_verts, n_cells);
  if (n_verts == 0 || n_cells == 0)
    return TriMesh::Ptr();
  TriMesh::Ptr mesh = TriMesh::Create(n_verts, n_cells);
  size_t node_index = 0;
  for (glm::vec3 pt : GetVertexIterator(scene)) {
    mesh->GetNode(node_index++) = pt;
  }
  size_t cell_index = 0;
  for (glm::u32vec3 tr : GetTriaIterator(scene)) {
    mesh->GetCell(cell_index++) = TriMesh::Cell(tr.x, tr.y, tr.z);
  }
  return mesh;
}
