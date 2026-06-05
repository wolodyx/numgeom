#include "numgeom/sceneobject.h"

#include <cassert>

#include "numgeom/drawable.h"
#include "numgeom/iteratorimpl.hpp"

#include "trackedobjectlist.h"

class SceneObjectImpl {
 public:
  Scene* scene;
  TrackedObjectList drawables;
};

SceneObject::SceneObject(Scene* scene) {
  assert(scene != nullptr);
  impl_ = new SceneObjectImpl {
    .scene = scene
  };
  this->AddSubObjects(&impl_->drawables);
}

SceneObject::~SceneObject() {
  delete impl_;
}

AlignedBoundBox SceneObject::GetBoundBox() const {
  AlignedBoundBox box;
  for (const Drawable* d : this->Drawables()) {
    box.Expand(d->GetBoundBox());
  }
  return box;
}

size_t SceneObject::DrawablesCount() const {
  return impl_->drawables.GetObjectsCount();
}

Iterator<Drawable*> SceneObject::Drawables() const {
  return GetObjects<Drawable>(impl_->drawables);
}

void SceneObject::Insert(Drawable* drawable) {
  impl_->drawables.Insert(drawable);
}

bool SceneObject::Remove(Drawable* drawable) {
  return impl_->drawables.Remove(drawable);
}
