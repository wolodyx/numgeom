#include "numgeom/axisindicator.h"

#include "numgeom/shapes.h"


TriMesh::Ptr Join(const std::vector<TriMesh::Ptr>& meshes) {
  size_t cells_count = 0, nodes_count = 0;
  for (const auto& m : meshes) {
    cells_count += m->NbCells();
    nodes_count += m->NbNodes();
  }
  TriMesh::Ptr result_mesh = TriMesh::Create(nodes_count, cells_count);
  size_t in = 0, ic = 0, node_offset = 0;
  for (const auto& m : meshes) {
    size_t nn = m->NbNodes();
    for (size_t i = 0; i < nn; ++i)
      result_mesh->GetNode(in++) = m->GetNode(i);
    size_t nc = m->NbCells();
    for (size_t i = 0; i < nc; ++i) {
      auto cell = m->GetCell(i);
      cell.na += node_offset;
      cell.nb += node_offset;
      cell.nc += node_offset;
      result_mesh->GetCell(ic++) = cell;
    }
    node_offset += nn;
  }
  return result_mesh;
}

TriMesh::Ptr GetAxisIndicatorMesh() {
  const glm::vec3 orig(0.0f, 0.0f, 0.0f);
  const glm::vec3 x_axis = glm::vec3(1.f, 0.f, 0.f);
  const glm::vec3 y_axis = glm::vec3(0.f, 1.f, 0.f);
  const glm::vec3 z_axis = glm::vec3(0.f, 0.f, 1.f);
  const float sphere_radius = 0.2f;
  const float axis_length = 1.0f;
  const float axis_thickness = 0.05f;
  const float cone_radius = 0.15f;
  const float cone_height = 0.3f;
  const size_t segments = 10;
  const glm::vec3 x_axis_end = orig + axis_length * x_axis;
  const glm::vec3 y_axis_end = orig + axis_length * y_axis;
  const glm::vec3 z_axis_end = orig + axis_length * z_axis;
  std::vector<TriMesh::Ptr> meshes = {
    MakeSphere(orig, sphere_radius, segments, segments),
    MakeCylinder(orig, x_axis_end, axis_thickness, segments),
    MakeCylinder(orig, y_axis_end, axis_thickness, segments),
    MakeCylinder(orig, z_axis_end, axis_thickness, segments),
    MakeCone(x_axis_end, x_axis_end + cone_height * x_axis, cone_radius,
             segments),
    MakeCone(y_axis_end, y_axis_end + cone_height * y_axis, cone_radius,
             segments),
    MakeCone(z_axis_end, z_axis_end + cone_height * z_axis, cone_radius,
             segments),
  };
  return Join(meshes);
}
