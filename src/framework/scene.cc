#include "numgeom/scene.h"

#include "numgeom/iteratorimpl.hpp"
#include "numgeom/sceneobject.h"

Scene::Scene() {
}

Scene::~Scene() {
}

AlignedBoundBox Scene::GetBoundBox() const {
  AlignedBoundBox box;
  for (SceneObject* o : objects_) {
    box.Expand(o->GetBoundBox());
  }
  return box;
}

void Scene::Clear() {
  if(objects_.empty())
    return;
  has_changes_ = true;
  for (SceneObject* o : objects_) {
    delete o;
  }
  objects_.clear();
}

bool Scene::HasChanges() const {
  if (has_changes_)
    return true;
  for (SceneObject* o : objects_) {
    if (o->HasChanges())
      return true;
  }
  return false;
}

void Scene::ClearChanges() {
  for (SceneObject* o : objects_)
    o->ClearChanges();
  has_changes_ = false;
}

Iterator<SceneObject*> Scene::Objects() const {
  typedef std::list<SceneObject*>::const_iterator StdIterType;
  auto it_impl = new IteratorImpl_StdIterator<StdIterType>(objects_.begin(),
                                                           objects_.end());
  return Iterator<SceneObject*>(it_impl);
}
