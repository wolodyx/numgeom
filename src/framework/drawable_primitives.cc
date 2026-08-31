#include "drawable_primitives.h"

#include "numgeom/iteratorimpl.hpp"

Drawable1_Segment::Drawable1_Segment(SceneObject* o)
    : Drawable(o), Drawable1(o) {
  start_ = glm::vec3(0.0f);
  end_ = glm::vec3(0.0f);
}

Drawable1_Segment::Drawable1_Segment(SceneObject* o,
                                     const glm::vec3& start,
                                     const glm::vec3& end)
    : Drawable(o), Drawable1(o) {
  start_ = start;
  end_ = end;
}

Drawable1_Segment::~Drawable1_Segment() {
}

Iterator<glm::u32vec2> Drawable1_Segment::GetEdges() const {
  auto it_impl = new IteratorImpl_OneValue<glm::u32vec2>(glm::u32vec2(0,1));
  return Iterator<glm::u32vec2>(it_impl);
}

size_t Drawable1_Segment::GetVertsCount() const { return 2; }

size_t Drawable1_Segment::GetCellsCount() const { return 1; }

Iterator<glm::vec3> Drawable1_Segment::GetVertices() const {
  auto it_impl = new IteratorImpl_ByEnum<glm::vec3>({start_,end_});
  return Iterator<glm::vec3>(it_impl);
}

void Drawable1_Segment::SetVertices(const glm::vec3& start,
                                    const glm::vec3& end) {
  start_ = start;
  end_ = end;
  this->SetDirty();
}
