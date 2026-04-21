#include "numgeom/drawable_cone.h"

#include <cmath>
#include <glm/gtc/constants.hpp>

namespace {

class ConeVertexIterator : public IteratorImpl<glm::vec3> {
 public:
  ConeVertexIterator(const glm::vec3& base_center, const glm::vec3& apex,
                     float radius, int segments, size_t index = 0)
      : base_center_(base_center), apex_(apex), radius_(radius),
        segments_(segments), current_index_(index) {}

  virtual ~ConeVertexIterator() {}

  void advance() override {
    ++current_index_;
  }

  glm::vec3 current() const override {
    return computeVertex(current_index_);
  }

  IteratorImpl<glm::vec3>* clone() const override {
    return new ConeVertexIterator(base_center_, apex_, radius_,
                                  segments_, current_index_);
  }

  IteratorImpl<glm::vec3>* last() const override {
    size_t total = static_cast<size_t>(segments_ + 1);
    return new ConeVertexIterator(base_center_, apex_, radius_,
                                  segments_, total);
  }

  bool end() const override {
    size_t total = static_cast<size_t>(segments_ + 1);
    return current_index_ == total;
  }

  bool equals(const IteratorImpl<glm::vec3>& other) const override {
    auto ptr = dynamic_cast<const ConeVertexIterator*>(&other);
    if (!ptr) return false;
    return base_center_ == ptr->base_center_ &&
           apex_ == ptr->apex_ &&
           radius_ == ptr->radius_ &&
           segments_ == ptr->segments_ &&
           current_index_ == ptr->current_index_;
  }

 private:
  glm::vec3 computeVertex(size_t index) const {
    size_t total = static_cast<size_t>(segments_ + 1);
    assert(index < total);

    // Apex vertex
    if (index == 0) {
      return apex_;
    }

    // Base vertices
    size_t base_index = index - 1;  // 0..segments_-1
    float angle = 2.0f * glm::pi<float>() * base_index / segments_;
    // Compute orthonormal basis u, v perpendicular to axis
    glm::vec3 axis = apex_ - base_center_;
    float height = glm::length(axis);
    if (height < 1e-6f) return base_center_; // degenerate cone
    glm::vec3 dir = axis / height;
    glm::vec3 u, v;
    if (fabs(dir.x) < 0.9f) {
      u = glm::cross(dir, glm::vec3(1.0f, 0.0f, 0.0f));
    } else {
      u = glm::cross(dir, glm::vec3(0.0f, 1.0f, 0.0f));
    }
    u = glm::normalize(u);
    v = glm::normalize(glm::cross(dir, u));

    glm::vec3 offset = radius_ * (std::cos(angle) * u + std::sin(angle) * v);
    return base_center_ + offset;
  }

 private:
  glm::vec3 base_center_;
  glm::vec3 apex_;
  float radius_;
  int segments_;
  size_t current_index_;
};

class ConeTriangleIterator : public IteratorImpl<glm::u32vec3> {
 public:
  ConeTriangleIterator(const glm::vec3& base_center, const glm::vec3& apex,
                       float radius, int segments, size_t index = 0)
      : base_center_(base_center), apex_(apex), radius_(radius),
        segments_(segments), current_index_(index) {}

  virtual ~ConeTriangleIterator() {}

  void advance() override {
    ++current_index_;
  }

  glm::u32vec3 current() const override {
    return computeTriangle(current_index_);
  }

  IteratorImpl<glm::u32vec3>* clone() const override {
    return new ConeTriangleIterator(base_center_, apex_, radius_,
                                    segments_, current_index_);
  }

  IteratorImpl<glm::u32vec3>* last() const override {
    size_t total = static_cast<size_t>(segments_);
    return new ConeTriangleIterator(base_center_, apex_, radius_,
                                    segments_, total);
  }

  bool end() const override {
    size_t total = static_cast<size_t>(segments_);
    return current_index_ == total;
  }

  bool equals(const IteratorImpl<glm::u32vec3>& other) const override {
    auto ptr = dynamic_cast<const ConeTriangleIterator*>(&other);
    if (!ptr) return false;
    return base_center_ == ptr->base_center_ &&
           apex_ == ptr->apex_ &&
           radius_ == ptr->radius_ &&
           segments_ == ptr->segments_ &&
           current_index_ == ptr->current_index_;
  }

 private:
  glm::u32vec3 computeTriangle(size_t index) const {
    size_t total = static_cast<size_t>(segments_);
    assert(index < total);

    // Triangle indices: apex (0), base vertex i+1, base vertex i+2 (wrapped)
    unsigned int apex = 0;
    unsigned int v1 = static_cast<unsigned int>(1 + index);
    unsigned int v2 = static_cast<unsigned int>(1 + (index + 1) % segments_);
    return glm::u32vec3(apex, v1, v2);
  }

 private:
  glm::vec3 base_center_;
  glm::vec3 apex_;
  float radius_;
  int segments_;
  size_t current_index_;
};

class ConeNormalIterator : public IteratorImpl<glm::vec3> {
 public:
  ConeNormalIterator(const glm::vec3& base_center, const glm::vec3& apex,
                     float radius, int segments, size_t index = 0)
      : base_center_(base_center), apex_(apex), radius_(radius),
        segments_(segments), current_index_(index) {}

  virtual ~ConeNormalIterator() {}

  void advance() override {
    ++current_index_;
  }

  glm::vec3 current() const override {
    return computeNormal(current_index_);
  }

  IteratorImpl<glm::vec3>* clone() const override {
    return new ConeNormalIterator(base_center_, apex_, radius_,
                                  segments_, current_index_);
  }

  IteratorImpl<glm::vec3>* last() const override {
    size_t total = static_cast<size_t>(segments_ + 1);
    return new ConeNormalIterator(base_center_, apex_, radius_,
                                  segments_, total);
  }

  bool end() const override {
    size_t total = static_cast<size_t>(segments_ + 1);
    return current_index_ == total;
  }

  bool equals(const IteratorImpl<glm::vec3>& other) const override {
    auto ptr = dynamic_cast<const ConeNormalIterator*>(&other);
    if (!ptr) return false;
    return base_center_ == ptr->base_center_ &&
           apex_ == ptr->apex_ &&
           radius_ == ptr->radius_ &&
           segments_ == ptr->segments_ &&
           current_index_ == ptr->current_index_;
  }

 private:
  glm::vec3 computeNormal(size_t index) const {
    size_t total = static_cast<size_t>(segments_ + 1);
    assert(index < total);

    // Apex vertex normal: average of all triangle normals
    if (index == 0) {
      // Compute axis and orthonormal basis
      glm::vec3 axis = apex_ - base_center_;
      float height = glm::length(axis);
      if (height < 1e-6f) return glm::vec3(0.0f, 0.0f, 1.0f);
      glm::vec3 dir = axis / height;
      glm::vec3 u, v;
      if (fabs(dir.x) < 0.9f) {
        u = glm::cross(dir, glm::vec3(1.0f, 0.0f, 0.0f));
      } else {
        u = glm::cross(dir, glm::vec3(0.0f, 1.0f, 0.0f));
      }
      u = glm::normalize(u);
      v = glm::normalize(glm::cross(dir, u));

      // Compute normal for one triangle (any) and assume symmetry
      float angle = 0.0f;
      glm::vec3 base_point = base_center_ + radius_ * (std::cos(angle) * u + std::sin(angle) * v);
      glm::vec3 edge1 = base_point - apex_;
      glm::vec3 edge2 = (base_center_ + radius_ * (std::cos(angle + 2.0f * glm::pi<float>() / segments_) * u +
                                                   std::sin(angle + 2.0f * glm::pi<float>() / segments_) * v)) - apex_;
      glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));
      return normal;
    }

    // Base vertex normal: normal of the side surface at that vertex
    size_t base_index = index - 1;  // 0..segments_-1
    float angle = 2.0f * glm::pi<float>() * base_index / segments_;
    // Compute orthonormal basis u, v perpendicular to axis
    glm::vec3 axis = apex_ - base_center_;
    float height = glm::length(axis);
    if (height < 1e-6f) return glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 dir = axis / height;
    glm::vec3 u, v;
    if (fabs(dir.x) < 0.9f) {
      u = glm::cross(dir, glm::vec3(1.0f, 0.0f, 0.0f));
    } else {
      u = glm::cross(dir, glm::vec3(0.0f, 1.0f, 0.0f));
    }
    u = glm::normalize(u);
    v = glm::normalize(glm::cross(dir, u));

    // Compute base point
    glm::vec3 base_point = base_center_ + radius_ * (std::cos(angle) * u + std::sin(angle) * v);
    // Vector from apex to base point
    glm::vec3 generatrix = base_point - apex_;
    // Tangent direction along the base circle
    glm::vec3 tangent = radius_ * (-std::sin(angle) * u + std::cos(angle) * v);
    // Surface normal is cross(tangent, generatrix) normalized (order depends on orientation)
    glm::vec3 normal = glm::normalize(glm::cross(tangent, generatrix));
    // Ensure normal points outward (should be perpendicular to axis)
    if (glm::dot(normal, dir) > 0) normal = -normal;
    return normal;
  }

 private:
  glm::vec3 base_center_;
  glm::vec3 apex_;
  float radius_;
  int segments_;
  size_t current_index_;
};

} // namespace

Drawable2_Cone::Drawable2_Cone(SceneObject* object,
                               const glm::vec3& base_center,
                               const glm::vec3& apex,
                               float radius, int segments)
    : Drawable2(object) {
  base_center_ = base_center;
  apex_ = apex;
  radius_ = radius;
  segments_ = segments;
}

Drawable2_Cone::~Drawable2_Cone() {
}

size_t Drawable2_Cone::GetVertsCount() const {
  return static_cast<size_t>(segments_ + 1);
}

size_t Drawable2_Cone::GetCellsCount() const {
  return static_cast<size_t>(segments_);
}

Iterator<glm::vec3> Drawable2_Cone::GetVertices() const {
  auto impl = new ConeVertexIterator(base_center_, apex_,
                                     radius_, segments_, 0);
  return Iterator<glm::vec3>(impl);
}

AlignedBoundBox Drawable2_Cone::GetBoundBox() const {
  // Compute bounding box that encloses base circle and apex
  glm::vec3 axis = apex_ - base_center_;
  float height = glm::length(axis);
  if (height < 1e-6f) {
    // Degenerate cone: just a point
    return AlignedBoundBox{base_center_, base_center_};
  }
  glm::vec3 dir = axis / height;
  glm::vec3 u, v;
  if (fabs(dir.x) < 0.9f) {
    u = glm::cross(dir, glm::vec3(1.0f, 0.0f, 0.0f));
  } else {
    u = glm::cross(dir, glm::vec3(0.0f, 1.0f, 0.0f));
  }
  u = glm::normalize(u);
  v = glm::normalize(glm::cross(dir, u));

  // Extreme points of the base circle in local coordinates
  glm::vec3 px = base_center_ + radius_ * u;
  glm::vec3 nx = base_center_ - radius_ * u;
  glm::vec3 py = base_center_ + radius_ * v;
  glm::vec3 ny = base_center_ - radius_ * v;

  glm::vec3 min = glm::min(apex_, glm::min(px, glm::min(nx, glm::min(py, ny))));
  glm::vec3 max = glm::max(apex_, glm::max(px, glm::max(nx, glm::max(py, ny))));
  return AlignedBoundBox{min, max};
}

Iterator<glm::u32vec3> Drawable2_Cone::GetTriangles() const {
  auto impl = new ConeTriangleIterator(base_center_, apex_,
                                       radius_, segments_, 0);
  return Iterator<glm::u32vec3>(impl);
}

Iterator<glm::vec3> Drawable2_Cone::GetNormals() const {
  auto impl = new ConeNormalIterator(base_center_, apex_,
                                     radius_, segments_, 0);
  return Iterator<glm::vec3>(impl);
}
