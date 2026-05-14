#include "numgeom/scene.h"

#include "numgeom/iteratorimpl.hpp"
#include "numgeom/sceneobject.h"

#include "scenestate.h"

Scene::Scene() {
  state_ = new State();
}

Scene::~Scene() {
  delete state_;
}

AlignedBoundBox Scene::GetBoundBox() const {
  AlignedBoundBox box;
  for (SceneObject* o : state_->objects_) {
    box.Expand(o->GetBoundBox());
  }
  return box;
}

void Scene::Clear() {
  if(state_->objects_.empty())
    return;
  state_->has_changes_ = true;
  for (SceneObject* o : state_->objects_) {
    delete o;
  }
  state_->objects_.clear();
}

bool Scene::HasChanges() const {
  if (state_->has_changes_)
    return true;
  for (SceneObject* o : state_->objects_) {
    if (o->HasChanges())
      return true;
  }
  return false;
}

void Scene::ClearChanges() {
  for (SceneObject* o : state_->objects_)
    o->ClearChanges();
  state_->has_changes_ = false;
}

Iterator<SceneObject*> Scene::Objects() const {
  typedef std::list<SceneObject*>::const_iterator StdIterType;
  auto it_impl = new IteratorImpl_StdIterator<StdIterType>(
      state_->objects_.begin(),
      state_->objects_.end());
  return Iterator<SceneObject*>(it_impl);
}

void Scene::AddObject(SceneObject* object) {
  state_->objects_.push_back(object);
  state_->has_changes_ = true;
}
