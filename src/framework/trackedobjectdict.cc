#include "trackedobjectdict.h"

#include "numgeom/trackedobject.h"

TrackedObjectDict::TrackedObjectDict() {
}

TrackedObjectDict::~TrackedObjectDict() {
  Clear();
  Synch();
}

bool TrackedObjectDict::Contains(const std::string& key) const {
  auto it = objects_.find(key);
  return it != objects_.end();
}

TrackedObject* TrackedObjectDict::Get(const std::string& key) const {
  auto it = objects_.find(key);
  if (it == objects_.end())
    return nullptr;
  return it->second;
}

bool TrackedObjectDict::Insert(const std::string& key, TrackedObject* object) {
  TrackedObject*& value = objects_[key];
  if (value != nullptr)
    return false;
  value = object;
  return true;
}

bool TrackedObjectDict::Remove(const std::string& key) {
  auto it = objects_.find(key);
  if (it == objects_.end())
    return false;
  it->second->MarkForDeletion();
  return true;
}

bool TrackedObjectDict::Remove(TrackedObject* object) {
  for (auto it = objects_.begin(); it != objects_.end(); ++it) {
    if (it->second == object) {
      object->MarkForDeletion();
      return true;
    }
  }
  return false;
}

void TrackedObjectDict::Clear() {
  for (auto& [key,object] : objects_) {
    if (object->GetState() != TrackedObject::State::Delete)
      object->MarkForDeletion();
  }
}

void TrackedObjectDict::Synch() {
  auto it = objects_.begin();
  while (it != objects_.end()) {
    auto it_current = it++;
    TrackedObject* o = it_current->second;
    if (o->GetState() == TrackedObject::State::Delete) {
      delete o;
      objects_.erase(it_current);
    }
  }
}
