#include "numgeom/drawableimplementations.h"

#include "numgeom/iteratorimpl.hpp"

Drawable_CachedPoints::Drawable_CachedPoints(SceneObject* object)
    : Drawable(object) {}

Drawable_CachedPoints::~Drawable_CachedPoints() {}

size_t Drawable_CachedPoints::GetVertsCount() const { return points_.size(); }

Iterator<glm::vec3> Drawable_CachedPoints::GetVertices() const {
  typedef std::vector<glm::vec3>::const_iterator IteratorType;
  IteratorImpl<glm::vec3>* it_impl = new IteratorImpl_StdIterator<IteratorType>(
      points_.begin(), points_.end());
  if (transform_function_) {
    it_impl = new IteratorImpl_Transform<glm::vec3,glm::vec3>(
        it_impl, transform_function_);
  }
  return Iterator<glm::vec3>(it_impl);
}

uint32_t Drawable_CachedPoints::AddPoint(const glm::vec3& point) {
  points_.push_back(point);
  this->SetDirty();
  return points_.size();
}

void Drawable_CachedPoints::ClearPoints() {
  points_.clear();
  this->SetDirty();
}

void Drawable_CachedPoints::SetTransformFunction(
    std::function<glm::vec3(glm::vec3)> f) {
  transform_function_ = f;
  this->SetDirty();
}

Drawable1_Cached::Drawable1_Cached(SceneObject* o)
    : Drawable(o), Drawable_CachedPoints(o), Drawable1(o) {}

Drawable1_Cached::~Drawable1_Cached() {}

size_t Drawable1_Cached::GetCellsCount() const { return cells_.size(); }

Iterator<glm::u32vec2> Drawable1_Cached::GetEdges() const {
  auto it_impl = new IteratorImpl_StdIterator<std::vector<glm::u32vec2>::const_iterator>(
      cells_.begin(), cells_.end());
  return Iterator<glm::u32vec2>(it_impl);
}

uint32_t Drawable1_Cached::AddCell(const glm::u32vec2& cell) {
  cells_.push_back(cell);
  this->SetDirty();
  return cells_.size();
}

uint32_t Drawable1_Cached::AddCell(uint32_t p1, uint32_t p2) {
  cells_.push_back({p1,p2});
  this->SetDirty();
  return cells_.size();
}

void Drawable1_Cached::ClearCells() {
  this->SetDirty();
  cells_.clear();
}

Drawable2_Cached::Drawable2_Cached(SceneObject* o)
    : Drawable(o), Drawable_CachedPoints(o), Drawable2(o) {}

Drawable2_Cached::~Drawable2_Cached() {}

size_t Drawable2_Cached::GetCellsCount() const { return cells_.size(); }

Iterator<glm::u32vec3> Drawable2_Cached::GetTriangles() const {
  auto it_impl = new IteratorImpl_StdIterator<std::vector<glm::u32vec3>::const_iterator>(
      cells_.begin(), cells_.end());
  return Iterator<glm::u32vec3>(it_impl);
}

Iterator<glm::vec3> Drawable2_Cached::GetNormals() const {
  return Iterator<glm::vec3>();
}

uint32_t Drawable2_Cached::AddCell(const glm::u32vec3& cell) {
  cells_.push_back(cell);
  this->SetDirty();
  return cells_.size();
}

uint32_t Drawable2_Cached::AddCell(uint32_t p1, uint32_t p2, uint32_t p3) {
  cells_.push_back({p1,p2,p3});
  this->SetDirty();
  return cells_.size();
}

void Drawable2_Cached::ClearCells() {
  this->SetDirty();
  cells_.clear();
}

Drawable0_Cached::Drawable0_Cached(SceneObject* o)
    : Drawable(o), Drawable0(o), Drawable_CachedPoints(o) {
}

Drawable0_Cached::~Drawable0_Cached() {
}

size_t Drawable0_Cached::GetCellsCount() const {
  return this->GetVertsCount();
}

void Drawable0_Cached::ClearCells() {
  this->ClearPoints();
}
