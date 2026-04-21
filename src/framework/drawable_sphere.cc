#include "numgeom/drawable_sphere.h"

#include <cmath>
#include <glm/gtc/constants.hpp>

namespace {

class SphereVertexIterator : public IteratorImpl<glm::vec3> {
 public:
  SphereVertexIterator(const glm::vec3& center, float radius,
                       int slices_num, int stacks_num, size_t index = 0)
      : center_(center), radius_(radius),
        slices_num_(slices_num), stacks_num_(stacks_num),
        current_index_(index) {}

  virtual ~SphereVertexIterator() {}

  void advance() override {
    ++current_index_;
  }

  glm::vec3 current() const override {
    return computeVertex(current_index_);
  }

  IteratorImpl<glm::vec3>* clone() const override {
    return new SphereVertexIterator(center_, radius_, slices_num_,
                                    stacks_num_, current_index_);
  }

  IteratorImpl<glm::vec3>* last() const override {
    size_t total = 2 + (stacks_num_ - 1) * slices_num_;
    return new SphereVertexIterator(center_, radius_, slices_num_,
                                    stacks_num_, total);
  }

  bool end() const override {
    size_t total = 2 + (stacks_num_ - 1) * slices_num_;
    return current_index_ == total;
  }

  bool equals(const IteratorImpl<glm::vec3>& other) const override {
    auto ptr = dynamic_cast<const SphereVertexIterator*>(&other);
    if (!ptr) return false;
    return center_ == ptr->center_ &&
           radius_ == ptr->radius_ &&
           slices_num_ == ptr->slices_num_ &&
           stacks_num_ == ptr->stacks_num_ &&
           current_index_ == ptr->current_index_;
  }

 private:
  glm::vec3 computeVertex(size_t index) const {
    size_t total = 2 + (stacks_num_ - 1) * slices_num_;
    assert(index < total);

    // South pole
    if (index == 0) {
      return center_ + glm::vec3(0.0f, 0.0f, -radius_);
    }

    // North pole
    if (index == total - 1) {
      return center_ + glm::vec3(0.0f, 0.0f, radius_);
    }

    // Internal vertices
    size_t internal_index = index - 1;  // after south pole
    size_t stack = internal_index / slices_num_;  // 0..stacks_num_-2
    size_t slice = internal_index % slices_num_;  // 0..slices_num_-1

    // u ranges from -π/2 to π/2, excluding poles
    // stack = 0 corresponds to first stack above south pole
    double u = (stack + 1) * glm::pi<double>() / stacks_num_ - glm::half_pi<double>();
    double cos_u = std::cos(u);
    double sin_u = std::sin(u);

    double v = 2.0 * glm::pi<double>() * slice / slices_num_;
    double cos_v = std::cos(v);
    double sin_v = std::sin(v);

    glm::vec3 p(radius_ * cos_u * cos_v,
                radius_ * cos_u * sin_v,
                radius_ * sin_u);
    return center_ + p;
  }

 private:
  glm::vec3 center_;
  float radius_;
  int slices_num_;
  int stacks_num_;
  size_t current_index_;
};

class SphereTriangleIterator : public IteratorImpl<glm::u32vec3> {
 public:
  SphereTriangleIterator(const glm::vec3& center, float radius,
                         int slices_num, int stacks_num, size_t index = 0)
      : center_(center), radius_(radius),
        slices_num_(slices_num), stacks_num_(stacks_num),
        current_index_(index) {}

  virtual ~SphereTriangleIterator() {}

  void advance() override {
    ++current_index_;
  }

  glm::u32vec3 current() const override {
    return computeTriangle(current_index_);
  }

  IteratorImpl<glm::u32vec3>* clone() const override {
    return new SphereTriangleIterator(center_, radius_, slices_num_,
                                      stacks_num_, current_index_);
  }

  IteratorImpl<glm::u32vec3>* last() const override {
    size_t total = 2 * slices_num_ + 2 * (stacks_num_ - 2) * slices_num_;
    return new SphereTriangleIterator(center_, radius_, slices_num_,
                                      stacks_num_, total);
  }

  bool end() const override {
    size_t total = 2 * slices_num_ + 2 * (stacks_num_ - 2) * slices_num_;
    return current_index_ == total;
  }

  bool equals(const IteratorImpl<glm::u32vec3>& other) const override {
    auto ptr = dynamic_cast<const SphereTriangleIterator*>(&other);
    if (!ptr) return false;
    return center_ == ptr->center_ &&
           radius_ == ptr->radius_ &&
           slices_num_ == ptr->slices_num_ &&
           stacks_num_ == ptr->stacks_num_ &&
           current_index_ == ptr->current_index_;
  }

 private:
  glm::u32vec3 computeTriangle(size_t index) const {
    size_t total = 2 * slices_num_ + 2 * (stacks_num_ - 2) * slices_num_;
    assert(index < total);

    size_t south_triangles = slices_num_;
    size_t internal_triangles = 2 * (stacks_num_ - 2) * slices_num_;
    size_t north_triangles = slices_num_;

    // South pole triangles
    if (index < south_triangles) {
      size_t i = index;
      unsigned int v0 = 0;
      unsigned int v1 = static_cast<unsigned int>(1 + (i + 1) % slices_num_);
      unsigned int v2 = static_cast<unsigned int>(i + 1);
      return glm::u32vec3(v0, v1, v2);
    }

    // Internal triangles
    if (index < south_triangles + internal_triangles) {
      size_t internal_index = index - south_triangles;
      size_t quad_index = internal_index / 2;
      size_t triangle_in_quad = internal_index % 2;

      size_t stack = quad_index / slices_num_;  // 0..stacks_num_-3
      size_t slice = quad_index % slices_num_;

      unsigned int downStartNode = static_cast<unsigned int>(1 + stack * slices_num_);
      unsigned int upStartNode = static_cast<unsigned int>(1 + (stack + 1) * slices_num_);

      unsigned int p00 = downStartNode + static_cast<unsigned int>(slice);
      unsigned int p01 = downStartNode + static_cast<unsigned int>((slice + 1) % slices_num_);
      unsigned int p10 = upStartNode + static_cast<unsigned int>(slice);
      unsigned int p11 = upStartNode + static_cast<unsigned int>((slice + 1) % slices_num_);

      if (triangle_in_quad == 0) {
        return glm::u32vec3(p00, p01, p11);
      } else {
        return glm::u32vec3(p00, p11, p10);
      }
    }

    // North pole triangles
    size_t north_index = index - (south_triangles + internal_triangles);
    size_t totalVertices = static_cast<size_t>(2 + (stacks_num_ - 1) * slices_num_);
    unsigned int poleNode = static_cast<unsigned int>(totalVertices - 1);
    unsigned int stackStartNode = static_cast<unsigned int>(poleNode - slices_num_);

    unsigned int v0 = poleNode;
    unsigned int v1 = stackStartNode + static_cast<unsigned int>(north_index);
    unsigned int v2 = stackStartNode + static_cast<unsigned int>((north_index + 1) % slices_num_);
    return glm::u32vec3(v0, v1, v2);
  }

 private:
  glm::vec3 center_;
  float radius_;
  int slices_num_;
  int stacks_num_;
  size_t current_index_;
};

class SphereNormalIterator : public IteratorImpl<glm::vec3> {
 public:
  SphereNormalIterator(const glm::vec3& center, float radius,
                       int slices_num, int stacks_num, size_t index = 0)
      : center_(center), radius_(radius),
        slices_num_(slices_num), stacks_num_(stacks_num),
        current_index_(index) {}

  virtual ~SphereNormalIterator() {}

  void advance() override {
    ++current_index_;
  }

  glm::vec3 current() const override {
    return computeNormal(current_index_);
  }

  IteratorImpl<glm::vec3>* clone() const override {
    return new SphereNormalIterator(center_, radius_, slices_num_,
                                    stacks_num_, current_index_);
  }

  IteratorImpl<glm::vec3>* last() const override {
    size_t total = 2 + (stacks_num_ - 1) * slices_num_;
    return new SphereNormalIterator(center_, radius_, slices_num_,
                                    stacks_num_, total);
  }

  bool end() const override {
    size_t total = 2 + (stacks_num_ - 1) * slices_num_;
    return current_index_ == total;
  }

  bool equals(const IteratorImpl<glm::vec3>& other) const override {
    auto ptr = dynamic_cast<const SphereNormalIterator*>(&other);
    if (!ptr) return false;
    return center_ == ptr->center_ &&
           radius_ == ptr->radius_ &&
           slices_num_ == ptr->slices_num_ &&
           stacks_num_ == ptr->stacks_num_ &&
           current_index_ == ptr->current_index_;
  }

 private:
  glm::vec3 computeNormal(size_t index) const {
    size_t total = 2 + (stacks_num_ - 1) * slices_num_;
    assert(index < total);

    // South pole
    if (index == 0) return glm::vec3(0.0f, 0.0f, -1.0f);

    // North pole
    if (index == total - 1) return glm::vec3(0.0f, 0.0f, 1.0f);

    // Internal vertices
    size_t internal_index = index - 1;  // after south pole
    size_t stack = internal_index / slices_num_;  // 0..stacks_num_-2
    size_t slice = internal_index % slices_num_;  // 0..slices_num_-1

    // u ranges from -π/2 to π/2, excluding poles
    double u = (stack + 1) * glm::pi<double>() / stacks_num_ - glm::half_pi<double>();
    double cos_u = std::cos(u);
    double sin_u = std::sin(u);

    double v = 2.0 * glm::pi<double>() * slice / slices_num_;
    double cos_v = std::cos(v);
    double sin_v = std::sin(v);

    return glm::vec3(cos_u * cos_v, cos_u * sin_v, sin_u);
  }

 private:
  glm::vec3 center_;
  float radius_;
  int slices_num_;
  int stacks_num_;
  size_t current_index_;
};

} // namespace

Drawable2_Sphere::Drawable2_Sphere(SceneObject* object, const glm::vec3& center,
                                  float radius, int slices_num, int stacks_num)
    : Drawable2(object) {
  center_ = center;
  radius_ = radius;
  slices_num_ = slices_num;
  stacks_num_ = stacks_num;
}

Drawable2_Sphere::~Drawable2_Sphere() {
}

size_t Drawable2_Sphere::GetVertsCount() const {
  return 2 + (stacks_num_ - 1) * slices_num_;
}

size_t Drawable2_Sphere::GetCellsCount() const {
  return 2 * slices_num_ + 2 * (stacks_num_ - 2) * slices_num_;
}

Iterator<glm::vec3> Drawable2_Sphere::GetVertices() const {
  auto impl = new SphereVertexIterator(center_, radius_,
                                       slices_num_, stacks_num_, 0);
  return Iterator<glm::vec3>(impl);
}

AlignedBoundBox Drawable2_Sphere::GetBoundBox() const {
  return AlignedBoundBox{center_ - glm::vec3{radius_},
                         center_ + glm::vec3{radius_}};
}

Iterator<glm::u32vec3> Drawable2_Sphere::GetTriangles() const {
  auto impl = new SphereTriangleIterator(center_, radius_,
                                         slices_num_, stacks_num_, 0);
  return Iterator<glm::u32vec3>(impl);
}

Iterator<glm::vec3> Drawable2_Sphere::GetNormals() const {
  auto impl = new SphereNormalIterator(center_, radius_,
                                       slices_num_, stacks_num_, 0);
  return Iterator<glm::vec3>(impl);
}
