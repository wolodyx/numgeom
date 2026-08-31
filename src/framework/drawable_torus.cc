#include "numgeom/drawable_torus.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <glm/gtc/constants.hpp>

namespace {

struct TorusParams {
  glm::vec3 center;
  glm::vec3 normal;
  float major_radius;
  float minor_radius;
  int segments;
  int rings;

  size_t verts_count() const {
    return static_cast<size_t>(segments) * static_cast<size_t>(rings);
  }

  size_t cells_count() const {
    return 2ull * static_cast<size_t>(segments) * static_cast<size_t>(rings);
  }
};

// Builds an orthonormal basis {ub, vb} perpendicular to the given direction.
// Returns false for a degenerate (zero-length) direction.
bool MakeBasis(const glm::vec3& dir, glm::vec3& ub, glm::vec3& vb) {
  float len = glm::length(dir);
  if (len < 1e-6f) return false;
  glm::vec3 n = dir / len;
  if (std::fabs(n.x) < 0.9f) {
    ub = glm::normalize(glm::cross(n, glm::vec3(1.0f, 0.0f, 0.0f)));
  } else {
    ub = glm::normalize(glm::cross(n, glm::vec3(0.0f, 1.0f, 0.0f)));
  }
  vb = glm::normalize(glm::cross(n, ub));
  return true;
}

glm::vec3 ComputeVertex(const TorusParams& p, size_t index) {
  size_t total = p.verts_count();
  assert(index < total);

  size_t i = index % static_cast<size_t>(p.segments);
  size_t j = index / static_cast<size_t>(p.segments);

  float u = 2.0f * glm::pi<float>() * static_cast<float>(i) /
            static_cast<float>(p.segments);
  float v = 2.0f * glm::pi<float>() * static_cast<float>(j) /
            static_cast<float>(p.rings);

  glm::vec3 dir = p.normal;
  glm::vec3 ub, vb;
  if (!MakeBasis(p.normal, ub, vb)) {
    dir = glm::vec3(0.0f, 0.0f, 1.0f);
    ub = glm::vec3(1.0f, 0.0f, 0.0f);
    vb = glm::vec3(0.0f, 1.0f, 0.0f);
  }

  glm::vec3 radial = std::cos(u) * ub + std::sin(u) * vb;
  return p.center +
         (p.major_radius + p.minor_radius * std::cos(v)) * radial +
         p.minor_radius * std::sin(v) * dir;
}

glm::vec3 ComputeNormal(const TorusParams& p, size_t index) {
  size_t total = p.verts_count();
  assert(index < total);

  size_t i = index % static_cast<size_t>(p.segments);
  size_t j = index / static_cast<size_t>(p.segments);

  float u = 2.0f * glm::pi<float>() * static_cast<float>(i) /
            static_cast<float>(p.segments);
  float v = 2.0f * glm::pi<float>() * static_cast<float>(j) /
            static_cast<float>(p.rings);

  glm::vec3 dir = p.normal;
  glm::vec3 ub, vb;
  if (!MakeBasis(p.normal, ub, vb)) {
    dir = glm::vec3(0.0f, 0.0f, 1.0f);
    ub = glm::vec3(1.0f, 0.0f, 0.0f);
    vb = glm::vec3(0.0f, 1.0f, 0.0f);
  }

  glm::vec3 radial = std::cos(u) * ub + std::sin(u) * vb;
  glm::vec3 normal = std::cos(v) * radial + std::sin(v) * dir;
  float len = glm::length(normal);
  if (len < 1e-6f) return glm::vec3(0.0f, 0.0f, 1.0f);
  return normal / len;
}

class TorusVertexIterator : public IteratorImpl<glm::vec3> {
 public:
  explicit TorusVertexIterator(const TorusParams& params, size_t index = 0)
      : params_(params), current_index_(index) {}

  virtual ~TorusVertexIterator() {}

  void advance() override { ++current_index_; }

  glm::vec3 current() const override { return ComputeVertex(params_, current_index_); }

  IteratorImpl<glm::vec3>* clone() const override {
    return new TorusVertexIterator(params_, current_index_);
  }

  IteratorImpl<glm::vec3>* last() const override {
    return new TorusVertexIterator(params_, params_.verts_count());
  }

  bool end() const override { return current_index_ == params_.verts_count(); }

  bool equals(const IteratorImpl<glm::vec3>& other) const override {
    auto ptr = dynamic_cast<const TorusVertexIterator*>(&other);
    if (!ptr) return false;
    return current_index_ == ptr->current_index_;
  }

 private:
  TorusParams params_;
  size_t current_index_;
};

class TorusTriangleIterator : public IteratorImpl<glm::u32vec3> {
 public:
  explicit TorusTriangleIterator(const TorusParams& params, size_t index = 0)
      : params_(params), current_index_(index) {}

  virtual ~TorusTriangleIterator() {}

  void advance() override { ++current_index_; }

  glm::u32vec3 current() const override { return ComputeTriangle(current_index_); }

  IteratorImpl<glm::u32vec3>* clone() const override {
    return new TorusTriangleIterator(params_, current_index_);
  }

  IteratorImpl<glm::u32vec3>* last() const override {
    return new TorusTriangleIterator(params_, params_.cells_count());
  }

  bool end() const override { return current_index_ == params_.cells_count(); }

  bool equals(const IteratorImpl<glm::u32vec3>& other) const override {
    auto ptr = dynamic_cast<const TorusTriangleIterator*>(&other);
    if (!ptr) return false;
    return current_index_ == ptr->current_index_;
  }

 private:
  glm::u32vec3 ComputeTriangle(size_t index) const {
    size_t total = params_.cells_count();
    assert(index < total);

    size_t cell = index / 2;
    size_t sub = index % 2;
    size_t i = cell % static_cast<size_t>(params_.segments);
    size_t j = cell / static_cast<size_t>(params_.segments);
    size_t ip1 = (i + 1) % static_cast<size_t>(params_.segments);
    size_t jp1 = (j + 1) % static_cast<size_t>(params_.rings);

    unsigned int v00 = static_cast<unsigned int>(i + j * static_cast<size_t>(params_.segments));
    unsigned int v10 = static_cast<unsigned int>(ip1 + j * static_cast<size_t>(params_.segments));
    unsigned int v01 = static_cast<unsigned int>(i + jp1 * static_cast<size_t>(params_.segments));
    unsigned int v11 = static_cast<unsigned int>(ip1 + jp1 * static_cast<size_t>(params_.segments));

    if (sub == 0) {
      return glm::u32vec3(v00, v10, v11);
    }
    return glm::u32vec3(v00, v11, v01);
  }

  TorusParams params_;
  size_t current_index_;
};

class TorusNormalIterator : public IteratorImpl<glm::vec3> {
 public:
  explicit TorusNormalIterator(const TorusParams& params, size_t index = 0)
      : params_(params), current_index_(index) {}

  virtual ~TorusNormalIterator() {}

  void advance() override { ++current_index_; }

  glm::vec3 current() const override { return ComputeNormal(params_, current_index_); }

  IteratorImpl<glm::vec3>* clone() const override {
    return new TorusNormalIterator(params_, current_index_);
  }

  IteratorImpl<glm::vec3>* last() const override {
    return new TorusNormalIterator(params_, params_.verts_count());
  }

  bool end() const override { return current_index_ == params_.verts_count(); }

  bool equals(const IteratorImpl<glm::vec3>& other) const override {
    auto ptr = dynamic_cast<const TorusNormalIterator*>(&other);
    if (!ptr) return false;
    return current_index_ == ptr->current_index_;
  }

 private:
  TorusParams params_;
  size_t current_index_;
};

} // namespace

Drawable2_Torus::Drawable2_Torus(SceneObject* o)
    : Drawable(o), Drawable2(o) {
  center_ = glm::vec3(0.0f, 0.0f, 0.0f);
  normal_ = glm::vec3(0.0f, 1.0f, 0.0f);
  major_radius_ = 0.0f;
  minor_radius_ = 0.0f;
  segments_ = 0;
  rings_ = 0;
}

Drawable2_Torus::Drawable2_Torus(SceneObject* o, const glm::vec3& center,
                                 const glm::vec3& normal, float major_radius,
                                 float minor_radius, int segments, int rings)
    : Drawable(o), Drawable2(o) {
  center_ = center;
  normal_ = normal;
  major_radius_ = major_radius;
  minor_radius_ = minor_radius;
  segments_ = segments;
  rings_ = rings;
}

Drawable2_Torus::~Drawable2_Torus() {}

size_t Drawable2_Torus::GetVertsCount() const {
  return static_cast<size_t>(segments_) * static_cast<size_t>(rings_);
}

size_t Drawable2_Torus::GetCellsCount() const {
  return 2ull * static_cast<size_t>(segments_) * static_cast<size_t>(rings_);
}

Iterator<glm::vec3> Drawable2_Torus::GetVertices() const {
  TorusParams params{center_, normal_, major_radius_, minor_radius_,
                     segments_, rings_};
  return Iterator<glm::vec3>(new TorusVertexIterator(params, 0));
}

AlignedBoundBox Drawable2_Torus::GetBoundBox() const {
  TorusParams params{center_, normal_, major_radius_, minor_radius_,
                     segments_, rings_};
  glm::vec3 min(0.0f), max(0.0f);
  bool first = true;
  for (size_t index = 0; index < params.verts_count(); ++index) {
    glm::vec3 p = ComputeVertex(params, index);
    if (first) {
      min = p;
      max = p;
      first = false;
    } else {
      min = glm::min(min, p);
      max = glm::max(max, p);
    }
  }
  return AlignedBoundBox{min, max};
}

Iterator<glm::u32vec3> Drawable2_Torus::GetTriangles() const {
  TorusParams params{center_, normal_, major_radius_, minor_radius_,
                     segments_, rings_};
  return Iterator<glm::u32vec3>(new TorusTriangleIterator(params, 0));
}

Iterator<glm::vec3> Drawable2_Torus::GetNormals() const {
  TorusParams params{center_, normal_, major_radius_, minor_radius_,
                     segments_, rings_};
  return Iterator<glm::vec3>(new TorusNormalIterator(params, 0));
}

void Drawable2_Torus::SetSegments(int segments) {
  if (segments_ == segments) return;
  segments_ = segments;
  this->SetDirty();
}

void Drawable2_Torus::SetRings(int rings) {
  if (rings_ == rings) return;
  rings_ = rings;
  this->SetDirty();
}

void Drawable2_Torus::UpdateParams(const glm::vec3& center,
                                   const glm::vec3& normal,
                                   float major_radius, float minor_radius) {
  center_ = center;
  normal_ = normal;
  major_radius_ = major_radius;
  minor_radius_ = minor_radius;
  this->SetDirty();
}
