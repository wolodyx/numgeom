#include "numgeom/drawable_cylinder.h"

#include <cmath>
#include <glm/gtc/constants.hpp>

namespace {

class CylinderVertexIterator : public IteratorImpl<glm::vec3> {
 public:
  CylinderVertexIterator(const glm::vec3& bottom_center,
                         const glm::vec3& top_center,
                         float radius, int segments, size_t index = 0)
      : bottom_center_(bottom_center), top_center_(top_center),
        radius_(radius), segments_(segments), current_index_(index) {}

  virtual ~CylinderVertexIterator() {}

  void advance() override {
    ++current_index_;
  }

  glm::vec3 current() const override {
    return computeVertex(current_index_);
  }

  IteratorImpl<glm::vec3>* clone() const override {
    return new CylinderVertexIterator(bottom_center_, top_center_,
                                      radius_, segments_, current_index_);
  }

  IteratorImpl<glm::vec3>* last() const override {
    size_t total = static_cast<size_t>(2 * segments_);
    return new CylinderVertexIterator(bottom_center_, top_center_,
                                      radius_, segments_, total);
  }

  bool end() const override {
    size_t total = static_cast<size_t>(2 * segments_);
    return current_index_ == total;
  }

  bool equals(const IteratorImpl<glm::vec3>& other) const override {
    auto ptr = dynamic_cast<const CylinderVertexIterator*>(&other);
    if (!ptr) return false;
    return bottom_center_ == ptr->bottom_center_ &&
           top_center_ == ptr->top_center_ &&
           radius_ == ptr->radius_ &&
           segments_ == ptr->segments_ &&
           current_index_ == ptr->current_index_;
  }

 private:
  glm::vec3 computeVertex(size_t index) const {
    size_t total = static_cast<size_t>(2 * segments_);
    assert(index < total);

    // Determine if it's a bottom (even) or top (odd) vertex
    bool is_bottom = (index % 2 == 0);
    int segment = static_cast<int>(index / 2);  // segment index 0..segments_-1
    float angle = 2.0f * glm::pi<float>() * segment / segments_;

    // Compute orthonormal basis u, v perpendicular to axis
    glm::vec3 axis = top_center_ - bottom_center_;
    float height = glm::length(axis);
    if (height < 1e-6f) return is_bottom ? bottom_center_ : top_center_;
    glm::vec3 dir = axis / height;
    glm::vec3 u, v;
    if (fabs(dir.x) < 0.9f) {
      u = glm::cross(dir, glm::vec3(1.0f, 0.0f, 0.0f));
    } else {
      u = glm::cross(dir, glm::vec3(0.0f, 1.0f, 0.0f));
    }
    u = glm::normalize(u);
    v = glm::normalize(glm::cross(u, dir));

    glm::vec3 offset = radius_ * (std::cos(angle) * u + std::sin(angle) * v);
    if (is_bottom) {
      return bottom_center_ + offset;
    } else {
      return top_center_ + offset;
    }
  }

 private:
  glm::vec3 bottom_center_;
  glm::vec3 top_center_;
  float radius_;
  int segments_;
  size_t current_index_;
};

class CylinderTriangleIterator : public IteratorImpl<glm::u32vec3> {
 public:
  CylinderTriangleIterator(const glm::vec3& bottom_center,
                           const glm::vec3& top_center,
                           float radius, int segments, size_t index = 0)
      : bottom_center_(bottom_center), top_center_(top_center),
        radius_(radius), segments_(segments), current_index_(index) {}

  virtual ~CylinderTriangleIterator() {}

  void advance() override {
    ++current_index_;
  }

  glm::u32vec3 current() const override {
    return computeTriangle(current_index_);
  }

  IteratorImpl<glm::u32vec3>* clone() const override {
    return new CylinderTriangleIterator(bottom_center_, top_center_,
                                        radius_, segments_, current_index_);
  }

  IteratorImpl<glm::u32vec3>* last() const override {
    size_t total = static_cast<size_t>(2 * segments_);
    return new CylinderTriangleIterator(bottom_center_, top_center_,
                                        radius_, segments_, total);
  }

  bool end() const override {
    size_t total = static_cast<size_t>(2 * segments_);
    return current_index_ == total;
  }

  bool equals(const IteratorImpl<glm::u32vec3>& other) const override {
    auto ptr = dynamic_cast<const CylinderTriangleIterator*>(&other);
    if (!ptr) return false;
    return bottom_center_ == ptr->bottom_center_ &&
           top_center_ == ptr->top_center_ &&
           radius_ == ptr->radius_ &&
           segments_ == ptr->segments_ &&
           current_index_ == ptr->current_index_;
  }

 private:
  glm::u32vec3 computeTriangle(size_t index) const {
    size_t total = static_cast<size_t>(2 * segments_);
    assert(index < total);

    // Two triangles per segment
    int segment = static_cast<int>(index / 2);  // segment index 0..segments_-1
    bool is_first_triangle = (index % 2 == 0);

    int i = segment;
    int i_next = (i + 1) % segments_;
    unsigned int low0 = static_cast<unsigned int>(2 * i);
    unsigned int top0 = static_cast<unsigned int>(2 * i + 1);
    unsigned int low1 = static_cast<unsigned int>(2 * i_next);
    unsigned int top1 = static_cast<unsigned int>(2 * i_next + 1);

    if (is_first_triangle) {
      // Triangle 1: (bottom_i, top_i, bottom_{i+1})
      return glm::u32vec3(low0, top0, low1);
    } else {
      // Triangle 2: (top_i, top_{i+1}, bottom_{i+1})
      return glm::u32vec3(top0, top1, low1);
    }
  }

 private:
  glm::vec3 bottom_center_;
  glm::vec3 top_center_;
  float radius_;
  int segments_;
  size_t current_index_;
};

class CylinderNormalIterator : public IteratorImpl<glm::vec3> {
 public:
  CylinderNormalIterator(const glm::vec3& bottom_center,
                         const glm::vec3& top_center,
                         float radius, int segments, size_t index = 0)
      : bottom_center_(bottom_center), top_center_(top_center),
        radius_(radius), segments_(segments), current_index_(index) {}

  virtual ~CylinderNormalIterator() {}

  void advance() override {
    ++current_index_;
  }

  glm::vec3 current() const override {
    return computeNormal(current_index_);
  }

  IteratorImpl<glm::vec3>* clone() const override {
    return new CylinderNormalIterator(bottom_center_, top_center_,
                                      radius_, segments_, current_index_);
  }

  IteratorImpl<glm::vec3>* last() const override {
    size_t total = static_cast<size_t>(2 * segments_);
    return new CylinderNormalIterator(bottom_center_, top_center_,
                                      radius_, segments_, total);
  }

  bool end() const override {
    size_t total = static_cast<size_t>(2 * segments_);
    return current_index_ == total;
  }

  bool equals(const IteratorImpl<glm::vec3>& other) const override {
    auto ptr = dynamic_cast<const CylinderNormalIterator*>(&other);
    if (!ptr) return false;
    return bottom_center_ == ptr->bottom_center_ &&
           top_center_ == ptr->top_center_ &&
           radius_ == ptr->radius_ &&
           segments_ == ptr->segments_ &&
           current_index_ == ptr->current_index_;
  }

 private:
  glm::vec3 computeNormal(size_t index) const {
    size_t total = static_cast<size_t>(2 * segments_);
    assert(index < total);

    // Normal depends only on segment (same for bottom and top vertices)
    int segment = static_cast<int>(index / 2);  // segment index 0..segments_-1
    float angle = 2.0f * glm::pi<float>() * segment / segments_;

    // Compute orthonormal basis u, v perpendicular to axis
    glm::vec3 axis = top_center_ - bottom_center_;
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
    v = glm::normalize(glm::cross(u, dir));

    // Outward normal vector
    return std::cos(angle) * u + std::sin(angle) * v;
  }

 private:
  glm::vec3 bottom_center_;
  glm::vec3 top_center_;
  float radius_;
  int segments_;
  size_t current_index_;
};

} // namespace

Drawable2_Cylinder::Drawable2_Cylinder(SceneObject* object,
                                       const glm::vec3& bottom_center,
                                       const glm::vec3& top_center,
                                       float radius, int segments)
    : Drawable2(object) {
  bottom_center_ = bottom_center;
  top_center_ = top_center;
  radius_ = radius;
  segments_ = segments;
}

Drawable2_Cylinder::~Drawable2_Cylinder() {
}

size_t Drawable2_Cylinder::GetVertsCount() const {
  return static_cast<size_t>(2 * segments_);
}

size_t Drawable2_Cylinder::GetCellsCount() const {
  return static_cast<size_t>(2 * segments_);
}

Iterator<glm::vec3> Drawable2_Cylinder::GetVertices() const {
  auto impl = new CylinderVertexIterator(bottom_center_, top_center_,
                                         radius_, segments_, 0);
  return Iterator<glm::vec3>(impl);
}

AlignedBoundBox Drawable2_Cylinder::GetBoundBox() const {
  // Compute bounding box that encloses both bottom and top circles
  glm::vec3 axis = top_center_ - bottom_center_;
  float height = glm::length(axis);
  if (height < 1e-6f) {
    // Degenerate cylinder: just a point
    return AlignedBoundBox{bottom_center_, bottom_center_};
  }
  glm::vec3 dir = axis / height;
  glm::vec3 u, v;
  if (fabs(dir.x) < 0.9f) {
    u = glm::cross(dir, glm::vec3(1.0f, 0.0f, 0.0f));
  } else {
    u = glm::cross(dir, glm::vec3(0.0f, 1.0f, 0.0f));
  }
  u = glm::normalize(u);
  v = glm::normalize(glm::cross(u, dir));

  // Extreme points of the bottom circle
  glm::vec3 px = bottom_center_ + radius_ * u;
  glm::vec3 nx = bottom_center_ - radius_ * u;
  glm::vec3 py = bottom_center_ + radius_ * v;
  glm::vec3 ny = bottom_center_ - radius_ * v;

  // Extreme points of the top circle
  glm::vec3 px_top = top_center_ + radius_ * u;
  glm::vec3 nx_top = top_center_ - radius_ * u;
  glm::vec3 py_top = top_center_ + radius_ * v;
  glm::vec3 ny_top = top_center_ - radius_ * v;

  glm::vec3 min = glm::min(bottom_center_, top_center_);
  min = glm::min(min, glm::min(px, glm::min(nx, glm::min(py, ny))));
  min = glm::min(min, glm::min(px_top, glm::min(nx_top, glm::min(py_top, ny_top))));

  glm::vec3 max = glm::max(bottom_center_, top_center_);
  max = glm::max(max, glm::max(px, glm::max(nx, glm::max(py, ny))));
  max = glm::max(max, glm::max(px_top, glm::max(nx_top, glm::max(py_top, ny_top))));

  return AlignedBoundBox{min, max};
}

Iterator<glm::u32vec3> Drawable2_Cylinder::GetTriangles() const {
  auto impl = new CylinderTriangleIterator(bottom_center_, top_center_,
                                           radius_, segments_, 0);
  return Iterator<glm::u32vec3>(impl);
}

Iterator<glm::vec3> Drawable2_Cylinder::GetNormals() const {
  auto impl = new CylinderNormalIterator(bottom_center_, top_center_,
                                         radius_, segments_, 0);
  return Iterator<glm::vec3>(impl);
}