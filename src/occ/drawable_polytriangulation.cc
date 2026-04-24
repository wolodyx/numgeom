#include "numgeom/drawable_polytriangulation.h"

#include "Poly_Triangulation.hxx"

#include "numgeom/iteratorimpl.hpp"

Drawable2_PolyTriangulation::Drawable2_PolyTriangulation(
    SceneObject* scene_object,
    Handle(Poly_Triangulation) triangulation) : Drawable2(scene_object) {
  triangulation_ = triangulation;
  if (!triangulation_->HasNormals()) {
    triangulation_->ComputeNormals();
  }
}

Drawable2_PolyTriangulation::~Drawable2_PolyTriangulation() {
}

size_t Drawable2_PolyTriangulation::GetVertsCount() const {
  return triangulation_->NbNodes();
}

size_t Drawable2_PolyTriangulation::GetCellsCount() const {
  return triangulation_->NbTriangles();
}

namespace {
class IteratorImpl_PolyTriangulatonNodes : public IteratorImpl<glm::vec3> {
 public:
  IteratorImpl_PolyTriangulatonNodes(const Poly_Triangulation* tr,
                                     Standard_Integer node_index = 0) {
    triangulation_ = tr;
    nodes_count_ = tr->NbNodes();
    node_index_ = node_index;
  }

  virtual ~IteratorImpl_PolyTriangulatonNodes() {}

  void advance() override {
    assert(node_index_ < nodes_count_);
    ++node_index_;
  }

  glm::vec3 current() const override {
    gp_Pnt pnt = triangulation_->Node(node_index_ + 1);
    return glm::vec3(pnt.X(), pnt.Y(), pnt.Z());
  }

  IteratorImpl<glm::vec3>* clone() const override {
    return new IteratorImpl_PolyTriangulatonNodes(triangulation_, node_index_);
  }

  IteratorImpl<glm::vec3>* last() const override {
    return new IteratorImpl_PolyTriangulatonNodes(triangulation_, nodes_count_);
  }

  bool end() const override {
    return node_index_ == nodes_count_;
  }

  bool equals(const IteratorImpl<glm::vec3>& other) const override {
    auto ptr = dynamic_cast<const IteratorImpl_PolyTriangulatonNodes*>(&other);
    if (!ptr)
      return false;
    return triangulation_ == ptr->triangulation_
        && node_index_ == ptr->node_index_;
  }

 private:
  const Poly_Triangulation* triangulation_;
  Standard_Integer nodes_count_;
  Standard_Integer node_index_;
};

class IteratorImpl_PolyTriangulatonTrias : public IteratorImpl<glm::u32vec3> {
 public:
  IteratorImpl_PolyTriangulatonTrias(const Poly_Triangulation* tr,
                                     Standard_Integer tria_index = 0) {
    triangulation_ = tr;
    trias_count_ = tr->NbTriangles();
    tria_index_ = tria_index;
  }

  virtual ~IteratorImpl_PolyTriangulatonTrias() {}

  void advance() override {
    assert(tria_index_ < trias_count_);
    ++tria_index_;
  }

  glm::u32vec3 current() const override {
    const Poly_Triangle& t = triangulation_->Triangle(tria_index_+1);
    Standard_Integer i1, i2, i3;
    t.Get(i1, i2, i3);
    return glm::u32vec3(i1-1, i2-1, i3-1);
  }

  IteratorImpl<glm::u32vec3>* clone() const override {
    return new IteratorImpl_PolyTriangulatonTrias(triangulation_, tria_index_);
  }

  IteratorImpl<glm::u32vec3>* last() const override {
    return new IteratorImpl_PolyTriangulatonTrias(triangulation_, trias_count_);
  }

  bool end() const override {
    return tria_index_ == trias_count_;
  }

  bool equals(const IteratorImpl<glm::u32vec3>& other) const override {
    auto ptr = dynamic_cast<const IteratorImpl_PolyTriangulatonTrias*>(&other);
    if (!ptr)
      return false;
    return triangulation_ == ptr->triangulation_
        && tria_index_ == ptr->tria_index_;
  }

 private:
  const Poly_Triangulation* triangulation_;
  Standard_Integer trias_count_;
  Standard_Integer tria_index_;
};

class IteratorImpl_PolyTriangulatonNormals : public IteratorImpl<glm::vec3> {
 public:
  IteratorImpl_PolyTriangulatonNormals(const Poly_Triangulation* tr,
                                       Standard_Integer node_index = 0) {
    triangulation_ = tr;
    nodes_count_ = tr->NbNodes();
    node_index_ = node_index;
  }

  virtual ~IteratorImpl_PolyTriangulatonNormals() {}

  void advance() override {
    assert(node_index_ < nodes_count_);
    ++node_index_;
  }

  glm::vec3 current() const override {
    if (!triangulation_->HasNormals())
      return glm::vec3(0,0,1);
    gp_Dir normal = triangulation_->Normal(node_index_ + 1);
    return glm::vec3(normal.X(), normal.Y(), normal.Z());
  }

  IteratorImpl<glm::vec3>* clone() const override {
    return new IteratorImpl_PolyTriangulatonNormals(triangulation_, node_index_);
  }

  IteratorImpl<glm::vec3>* last() const override {
    return new IteratorImpl_PolyTriangulatonNormals(triangulation_, nodes_count_);
  }

  bool end() const override {
    return node_index_ == nodes_count_;
  }

  bool equals(const IteratorImpl<glm::vec3>& other) const override {
    auto ptr = dynamic_cast<const IteratorImpl_PolyTriangulatonNormals*>(&other);
    if (!ptr)
      return false;
    return triangulation_ == ptr->triangulation_
        && node_index_ == ptr->node_index_;
  }

 private:
  const Poly_Triangulation* triangulation_;
  Standard_Integer nodes_count_;
  Standard_Integer node_index_;
};
}

Iterator<glm::vec3> Drawable2_PolyTriangulation::GetVertices() const {
  auto impl = new IteratorImpl_PolyTriangulatonNodes(triangulation_.get());
  return Iterator<glm::vec3>(impl);
}

Iterator<glm::u32vec3> Drawable2_PolyTriangulation::GetTriangles() const {
  auto impl = new IteratorImpl_PolyTriangulatonTrias(triangulation_.get());
  return Iterator<glm::u32vec3>(impl);
}

Iterator<glm::vec3> Drawable2_PolyTriangulation::GetNormals() const {
  auto impl = new IteratorImpl_PolyTriangulatonNormals(triangulation_.get());
  return Iterator<glm::vec3>(impl);
}
