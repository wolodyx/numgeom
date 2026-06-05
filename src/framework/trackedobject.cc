#include "numgeom/trackedobject.h"

#include <cassert>

#include "trackedobjectlist.h"

TrackedObject::TrackedObject() {
}

TrackedObject::~TrackedObject() {
  //assert(object_state_ == State::Delete);
}

TrackedObject::State TrackedObject::GetState() const {
  if (!sub_objects_)
    return object_state_;
  if (object_state_ != State::Clean)
    return object_state_;
  for (TrackedObject* sub_object : sub_objects_->GetObjects()) {
    auto sub_object_state = sub_object->GetState();
    if (sub_object_state == State::New || sub_object_state == State::Dirty)
      return State::Dirty;
  }
  return State::Clean;
}

void TrackedObject::SetDirty() {
  assert(object_state_ != State::Removed);
  if (object_state_ != State::New)
    object_state_ = State::Dirty;
}

void TrackedObject::Sync() {
  assert(object_state_ != State::Delete);
  switch (object_state_) {
  case State::New:
    object_state_ = State::Clean; break;
  case State::Clean:
    break;
  case State::Dirty:
    object_state_ = State::Clean; break;
  case State::Removed:
    object_state_ = State::Delete; break;
  case State::Delete:
    std::abort();
  }
  if (sub_objects_) {
    for (TrackedObject* sub_object : sub_objects_->GetObjects())
      sub_object->Sync();
  }
}

void TrackedObject::MarkForDeletion() {
  assert(object_state_ != State::Removed && object_state_ != State::Delete);
  if (object_state_ == State::New)
    object_state_ = State::Delete;
  else
    object_state_ = State::Removed;
}

void TrackedObject::AddSubObjects(const TrackedObjectList* sub_objects) {
  sub_objects_ = sub_objects;
}
