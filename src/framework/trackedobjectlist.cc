#include "trackedobjectlist.h"

#include "numgeom/iteratorimpl.hpp"
#include "numgeom/trackedobject.h"

TrackedObjectList::TrackedObjectList() {
}

TrackedObjectList::~TrackedObjectList() {
  Clear();
  Synch();
}

bool TrackedObjectList::IsEmpty() const {
  return GetObjectsCount() == 0;
}

size_t TrackedObjectList::GetObjectsCount() const {
  size_t count = 0;
  for (TrackedObject* o : objects_) {
    if (o->GetState() != TrackedObject::State::Delete)
      ++count;
  }
  return count;
}

Iterator<TrackedObject*> TrackedObjectList::GetObjects() const {
  typedef std::list<TrackedObject*>::const_iterator StdIteratorType;
  auto it_impl = new IteratorImpl_StdIterator<StdIteratorType>(objects_.begin(),
                                                               objects_.end());
  struct NonDeletedObjectFilter {
    bool operator()(const TrackedObject* o) const {
      return o->GetState() != TrackedObject::State::Delete;
    }
  };
  auto it_filter_impl = new IteratorImpl_Filter<TrackedObject*,
                                                NonDeletedObjectFilter>(it_impl);
  return Iterator<TrackedObject*>(it_filter_impl);
}

Iterator<TrackedObject*> TrackedObjectList::GetAllObjects() const {
  typedef std::list<TrackedObject*>::const_iterator StdIteratorType;
  auto impl = new IteratorImpl_StdIterator<StdIteratorType>(objects_.begin(),
                                                            objects_.end());
  return Iterator<TrackedObject*>(impl);
}

void TrackedObjectList::Insert(TrackedObject* object) {
  for (TrackedObject* o : objects_) {
    if (o == object) {
#ifndef NDEBUG
      std::abort();
#endif // !NDEBUG
      return;
    }
  }
  objects_.push_back(object);
}

bool TrackedObjectList::Remove(TrackedObject* object) {
  for (TrackedObject* o : objects_) {
    if (o == object) {
      o->MarkForDeletion();
      return true;
    }
  }
  return false;
}

void TrackedObjectList::Clear() {
  for (TrackedObject* o : objects_) {
    if (o->GetState() != TrackedObject::State::Delete)
      o->MarkForDeletion();
  }
}

void TrackedObjectList::Synch() {
  auto it = objects_.begin();
  while (it != objects_.end()) {
    auto it_current = it++;
    TrackedObject* o = (*it_current);
    if (o->GetState() == TrackedObject::State::Delete) {
      delete o;
      objects_.erase(it_current);
    }
  }
}
