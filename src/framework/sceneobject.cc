#include "numgeom/sceneobject.h"

#include <cassert>

#include "numgeom/drawable.h"
#include "numgeom/iteratorimpl.hpp"

SceneObject::SceneObject(Scene* scene) {
  assert(scene != nullptr);
  scene_ = scene;
}

SceneObject::~SceneObject() {
}

AlignedBoundBox SceneObject::GetBoundBox() const {
  AlignedBoundBox box;
  for (const Drawable* d : this->Drawables()) {
    box.Expand(d->GetBoundBox());
  }
  return box;
}

size_t SceneObject::DrawablesCount() const {
  return drawables_.size();
}

Iterator<Drawable*> SceneObject::Drawables() const {
  typedef std::vector<Drawable*>::const_iterator StdIterType;
  auto it_pimpl = new IteratorImpl_StdIterator<StdIterType>(drawables_.begin(),
                                                            drawables_.end());
  return Iterator<Drawable*>(it_pimpl);
}

bool SceneObject::Remove(Drawable* drawable) {
  for (auto it = drawables_.begin(); it != drawables_.end(); ++it) {
    Drawable* d = (*it);
    if (d != drawable) continue;
    delete drawable;
    drawables_.erase(it);
    has_changes_ = true;
    return true;
  }
  return false;
}

bool SceneObject::HasChanges() const {
  return has_changes_;
}

void SceneObject::ClearChanges() {
  for (auto* d : drawables_)
    d->ClearChanges();
  has_changes_ = false;
}
