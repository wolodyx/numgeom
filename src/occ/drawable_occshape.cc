#include "numgeom/drawable_occshape.h"

#include "BRep_Tool.hxx"
#include "BRepLib_ToolTriangulatedShape.hxx"
#include "BRepMesh_IncrementalMesh.hxx"
#include "gp_Trsf.hxx"
#include "TopExp_Explorer.hxx"
#include "TopoDS.hxx"
#include "TopoDS_Face.hxx"
#include "TopoDS_Shape.hxx"

#include "glm/gtc/matrix_transform.hpp"

namespace {

class OccVertexIterator : public IteratorImpl<glm::vec3> {
 public:
  typedef std::tuple<Handle(Poly_Triangulation),gp_Trsf,bool> TrngInfo;

 public:
  OccVertexIterator(const std::list<TrngInfo>& triangulations)
      : triangulations_(triangulations),
        trng_it_(triangulations_.end()),
        current_index_(0),
        index_count_(0) {
    if(!triangulations_.empty()) {
      trng_it_ = triangulations_.begin();
      index_count_ = std::get<0>(*trng_it_)->NbNodes();
    }
  }

  OccVertexIterator(const std::list<TrngInfo>& triangulations,
                    std::list<TrngInfo>::const_iterator trng_it,
                    size_t current_index = 0,
                    size_t index_count = 0)
      : triangulations_(triangulations),
        trng_it_(trng_it),
        current_index_(current_index),
        index_count_(index_count) {
  }

  virtual ~OccVertexIterator() {}

  void advance() override {
    ++current_index_;
    if (current_index_ == index_count_) {
      ++trng_it_;
      if(trng_it_ == triangulations_.end())
        return;
      current_index_ = 0;
      index_count_ = std::get<0>(*trng_it_)->NbNodes();
    }
  }

  glm::vec3 current() const override {
    gp_Pnt pt = std::get<0>(*trng_it_)->Node(current_index_ + 1);
    pt.Transform(std::get<1>(*trng_it_));
    return glm::vec3(pt.X(), pt.Y(), pt.Z());
  }

  IteratorImpl<glm::vec3>* clone() const override {
    return new OccVertexIterator(triangulations_, trng_it_, current_index_,
                                 index_count_);
  }

  IteratorImpl<glm::vec3>* last() const override {
    return new OccVertexIterator(triangulations_, triangulations_.end());
  }

  bool end() const override {
    return trng_it_ == triangulations_.end();
  }

  bool equals(const IteratorImpl<glm::vec3>& other) const override {
    auto ptr = dynamic_cast<const OccVertexIterator*>(&other);
    if (!ptr) return false;
    if(trng_it_ == triangulations_.end() && trng_it_ == ptr->trng_it_)
      return true;
    return trng_it_ == ptr->trng_it_ &&
           current_index_ == ptr->current_index_;
  }

 private:
  const std::list<TrngInfo>& triangulations_;
  std::list<TrngInfo>::const_iterator trng_it_;
  size_t current_index_;
  size_t index_count_;
};

class OccTriangleIterator : public IteratorImpl<glm::u32vec3> {
 public:
   typedef std::tuple<Handle(Poly_Triangulation),gp_Trsf,bool> TrngInfo;

 public:
   OccTriangleIterator(const std::list<TrngInfo>& triangulations)
       : triangulations_(triangulations),
         current_index_(0),
         index_count_(0),
         vertex_offset_(0),
         reverse_normal_(false) {
     if(!triangulations_.empty()) {
       trng_it_ = triangulations_.begin();
       index_count_ = std::get<0>(*trng_it_)->NbTriangles();
       reverse_normal_ = std::get<2>(*trng_it_);
     }
   }

  OccTriangleIterator(const std::list<TrngInfo>& triangulations,
                     std::list<TrngInfo>::const_iterator trng_it,
                     size_t current_index = 0,
                     size_t index_count = 0,
                     size_t vertex_offset = 0)
      : triangulations_(triangulations),
        trng_it_(trng_it),
        current_index_(current_index),
        index_count_(index_count),
        vertex_offset_(vertex_offset),
        reverse_normal_(false) {
    if(trng_it_ != triangulations_.end())
      reverse_normal_ = std::get<2>(*trng_it_);
  }

   virtual ~OccTriangleIterator() {}

  void advance() override {
    ++current_index_;
    if (current_index_ == index_count_) {
      vertex_offset_ += std::get<0>(*trng_it_)->NbNodes();
      ++trng_it_;
      current_index_ = 0;
      if (trng_it_ != triangulations_.end()) {
        index_count_ = std::get<0>(*trng_it_)->NbTriangles();
        reverse_normal_ = std::get<2>(*trng_it_);
      }
    }
  }

  glm::u32vec3 current() const override {
    const Handle(Poly_Triangulation)& trng = std::get<0>(*trng_it_);
    const Poly_Triangle& tri = trng->Triangle(current_index_ + 1); // 1-based index
    Standard_Integer na, nb, nc;
    tri.Get(na, nb, nc);
    // Convert to 0-based and add vertex offset
    glm::u32vec3 tr(
        static_cast<uint32_t>(na - 1 + vertex_offset_),
        static_cast<uint32_t>(nb - 1 + vertex_offset_),
        static_cast<uint32_t>(nc - 1 + vertex_offset_));
    if (reverse_normal_)
      std::swap(tr.y, tr.z);
    return tr;
  }

  IteratorImpl<glm::u32vec3>* clone() const override {
    return new OccTriangleIterator(triangulations_, trng_it_, current_index_,
                                   index_count_, vertex_offset_);
  }

  IteratorImpl<glm::u32vec3>* last() const override {
    return new OccTriangleIterator(triangulations_, triangulations_.end());
  }

  bool end() const override {
    return trng_it_ == triangulations_.end();
  }

  bool equals(const IteratorImpl<glm::u32vec3>& other) const override {
    auto ptr = dynamic_cast<const OccTriangleIterator*>(&other);
    if (!ptr) return false;
    return trng_it_ == ptr->trng_it_ &&
           current_index_ == ptr->current_index_ &&
           index_count_ == ptr->index_count_;
  }

 private:
  const std::list<TrngInfo>& triangulations_;
  std::list<TrngInfo>::const_iterator trng_it_;
  size_t current_index_;
  size_t index_count_;
  size_t vertex_offset_;
  bool reverse_normal_;
};

class OccNormalIterator : public IteratorImpl<glm::vec3> {
 public:
   typedef std::tuple<Handle(Poly_Triangulation),gp_Trsf,bool> TrngInfo;

 public:
  OccNormalIterator(const std::list<TrngInfo>& triangulations)
      : triangulations_(triangulations),
        trng_it_(triangulations_.end()),
        current_index_(0),
        index_count_(0),
        reverse_normal_(false) {
    if(!triangulations_.empty()) {
      trng_it_ = triangulations_.begin();
      index_count_ = std::get<0>(*trng_it_)->NbNodes();
      reverse_normal_ = std::get<2>(*trng_it_);
    }
  }

  OccNormalIterator(const std::list<TrngInfo>& triangulations,
                    std::list<TrngInfo>::const_iterator trng_it,
                    size_t current_index = 0,
                    size_t index_count = 0)
      : triangulations_(triangulations),
        trng_it_(trng_it),
        current_index_(current_index),
        index_count_(index_count),
        reverse_normal_(false) {
    if(trng_it_ != triangulations_.end())
      reverse_normal_ = std::get<2>(*trng_it_);
  }

  virtual ~OccNormalIterator() {}

  void advance() override {
    ++current_index_;
    if (current_index_ == index_count_) {
      ++trng_it_;
      if(trng_it_ == triangulations_.end())
        return;
      current_index_ = 0;
      index_count_ = std::get<0>(*trng_it_)->NbNodes();
      reverse_normal_ = std::get<2>(*trng_it_);
    }
  }

  glm::vec3 current() const override {
    const Handle(Poly_Triangulation)& trng = std::get<0>(*trng_it_);
    gp_Dir normal;
    if (!trng->HasNormals())
      return glm::vec3(0.0f, 0.0f, 1.0f);
    normal = trng->Normal(current_index_ + 1);
    if (reverse_normal_)
      normal.Reverse();
    normal.Transform(std::get<1>(*trng_it_));
    return glm::vec3(normal.X(), normal.Y(), normal.Z());
  }

  IteratorImpl<glm::vec3>* clone() const override {
    return new OccNormalIterator(triangulations_, trng_it_, current_index_,
                                 index_count_);
  }

  IteratorImpl<glm::vec3>* last() const override {
    return new OccNormalIterator(triangulations_, triangulations_.end());
  }

  bool end() const override {
    return trng_it_ == triangulations_.end();
  }

  bool equals(const IteratorImpl<glm::vec3>& other) const override {
    auto ptr = dynamic_cast<const OccNormalIterator*>(&other);
    if (!ptr) return false;
    return trng_it_ == ptr->trng_it_ &&
           current_index_ == ptr->current_index_;
  }

  private:
   const std::list<TrngInfo>& triangulations_;
   std::list<TrngInfo>::const_iterator trng_it_;
   size_t current_index_;
   size_t index_count_;
   bool reverse_normal_;
};

} // namespace

Drawable2_OccShape::Drawable2_OccShape(SceneObject* parent,
                                       const TopoDS_Shape& shape,
                                       const gp_Trsf& trsf)
    : Drawable2(parent), shape_(shape), trsf_(trsf) {
  verts_num_ = 0;
  cells_num_ = 0;
  for (TopExp_Explorer it(shape,TopAbs_FACE); it.More(); it.Next()) {
    const TopoDS_Face& f = TopoDS::Face(it.Value());
    TopLoc_Location l;
    Handle(Poly_Triangulation) tr = BRep_Tool::Triangulation(f,l);
    if (!tr) {
      BRepMesh_IncrementalMesh(f,0.6f);
      tr = BRep_Tool::Triangulation(f,l);
    }
    if (!tr)
      continue;
    if (!tr->HasNormals() && tr->HasUVNodes()) {
      BRepLib_ToolTriangulatedShape::ComputeNormals(f,tr);
    }
    verts_num_ += tr->NbNodes();
    cells_num_ += tr->NbTriangles();
    bool reverse_normals = (f.Orientation() == TopAbs_REVERSED);
    triangulations_.push_back(std::make_tuple(tr,l*trsf,reverse_normals));
  }
}

Drawable2_OccShape::~Drawable2_OccShape() {
}

size_t Drawable2_OccShape::GetVertsCount() const {
  return verts_num_;
}

size_t Drawable2_OccShape::GetCellsCount() const {
  return cells_num_;
}

Iterator<glm::vec3> Drawable2_OccShape::GetVertices() const {
  auto impl = new OccVertexIterator(triangulations_);
  return Iterator<glm::vec3>(impl);
}

Iterator<glm::u32vec3> Drawable2_OccShape::GetTriangles() const {
  auto impl = new OccTriangleIterator(triangulations_);
  return Iterator<glm::u32vec3>(impl);
}

Iterator<glm::vec3> Drawable2_OccShape::GetNormals() const {
  auto impl = new OccNormalIterator(triangulations_);
  return Iterator<glm::vec3>(impl);
}
