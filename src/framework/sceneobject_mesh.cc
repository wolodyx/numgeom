#include "numgeom/sceneobject_mesh.h"

#include "numgeom/drawable.h"
#include "numgeom/iteratorimpl.hpp"
#include "numgeom/trimeshconnectivity.h"

namespace {
struct DVec3ToVec3 {
  glm::vec3 operator()(const glm::dvec3& v) const {
    return glm::vec3(v.x, v.y, v.z);
  }
};

glm::vec3 computeTriangleNormal(const glm::vec3& a, const glm::vec3& b,
                                const glm::vec3& c) {
  glm::vec3 ab = b - a;
  glm::vec3 ac = c - a;
  glm::vec3 normal = glm::cross(ab, ac);
  float length = glm::length(normal);
  if (length < 1e-6f)
    return glm::vec3();
  return normal / length;
}

glm::vec3 computeNormal(CTriMesh::Ptr mesh, size_t vertex_index) {
  std::vector<size_t> adj_trias;
  glm::vec3 node_n(0.0f, 0.0f, 0.0f);
  TriMeshConnectivity* con = mesh->Connectivity();
  con->Node2Trias(vertex_index, adj_trias);
  for (size_t tria_i : adj_trias) {
    const auto& tria = mesh->GetCell(tria_i);
    glm::vec3 tria_n = computeTriangleNormal(mesh->GetNode(tria.na),
                                             mesh->GetNode(tria.nb),
                                             mesh->GetNode(tria.nc));
    node_n += tria_n;
  }
  return glm::normalize(node_n);
}

class IteratorImpl_DrawableNormals : public IteratorImpl<glm::vec3> {
 public:

  IteratorImpl_DrawableNormals(CTriMesh::Ptr mesh,
                               size_t vertex_index = 0) {
    mesh_ = mesh;
    vertex_index_ = vertex_index;
  }

  virtual ~IteratorImpl_DrawableNormals() {}

  void advance() override {
    ++vertex_index_;
  }

  glm::vec3 current() const override {
    return computeNormal(mesh_, vertex_index_);
  }

  IteratorImpl<glm::vec3>* clone() const override {
    return new IteratorImpl_DrawableNormals(mesh_, vertex_index_);
  }

  IteratorImpl<glm::vec3>* last() const override {
    return new IteratorImpl_DrawableNormals(mesh_, mesh_->NbNodes());
  }

  bool end() const override {
    return vertex_index_ == mesh_->NbNodes();
  }

  bool equals(const IteratorImpl<glm::vec3>& other) const override {
    auto ptr = dynamic_cast<const IteratorImpl_DrawableNormals*>(&other);
    if (!ptr)
      return false;
    return mesh_ == ptr->mesh_ && vertex_index_ == ptr->vertex_index_;
  }

 private:
  CTriMesh::Ptr mesh_;
  size_t vertex_index_;
};

struct TriCellToU32Vec3 {
  glm::u32vec3 operator()(const TriMesh::Cell& cell) const {
    return glm::u32vec3(cell.na, cell.nb, cell.nc);
  }
};

class Drawable2_TriMesh : public Drawable2 {
 public:

  Drawable2_TriMesh(SceneObject* o, CTriMesh::Ptr mesh) : Drawable2(o) {
    mesh_ = mesh;
  }

  virtual ~Drawable2_TriMesh() {}

  size_t GetVertsCount() const override { return mesh_->NbNodes(); }

  size_t GetCellsCount() const override { return mesh_->NbCells(); }

  AlignedBoundBox GetBoundBox() const override {
    AlignedBoundBox box;
    for (size_t i = 0; i < mesh_->NbNodes(); ++i) {
      const auto& pt = mesh_->GetNode(i);
      box.Expand(pt);
    }
    return box;
  }

  Iterator<glm::vec3> GetVertices() const override {
    auto impl = new IteratorImpl_ByIndex<CTriMesh,
                                         glm::dvec3,
                                         &CTriMesh::NbNodes,
                                         &CTriMesh::GetNode>(mesh_.get());
    auto impl2 = new IteratorImpl_Transform<glm::dvec3, glm::vec3, DVec3ToVec3>(impl);
    return Iterator<glm::vec3>(impl2);
  }

  Iterator<glm::u32vec3> GetTriangles() const override {
    auto impl = new IteratorImpl_ByIndex<CTriMesh,
                                         CTriMesh::Cell,
                                         &CTriMesh::NbCells,
                                         &CTriMesh::GetCell>(mesh_.get());
    auto impl2 = new IteratorImpl_Transform<CTriMesh::Cell, glm::u32vec3, TriCellToU32Vec3>(impl);
    return Iterator<glm::u32vec3>(impl2);
  }

  Iterator<glm::vec3> GetNormals() const override {
    auto impl = new IteratorImpl_DrawableNormals(mesh_);
    return Iterator<glm::vec3>(impl);
  }

 private:
  CTriMesh::Ptr mesh_;
};
}

SceneObject_Mesh::SceneObject_Mesh(Scene* scene, CTriMesh::Ptr mesh)
    : SceneObject(scene) {
  if(mesh) {
    this->AddDrawable<Drawable2_TriMesh>(mesh);
  }
}

SceneObject_Mesh::~SceneObject_Mesh() {
}
