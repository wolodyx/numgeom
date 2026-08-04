#ifndef NUMGEOM_FRAMEWORK_TRACKEDOBJECTDICT_H
#define NUMGEOM_FRAMEWORK_TRACKEDOBJECTDICT_H

#include <map>
#include <string>

#include "numgeom/iterator.h"
#include "numgeom/iteratorimpl.h"

class TrackedObject;

class TrackedObjectDict {
 public:
  TrackedObjectDict();
  ~TrackedObjectDict();

  bool Contains(const std::string&) const;

  TrackedObject* Get(const std::string&) const;

  bool Insert(const std::string& key, TrackedObject* value);

  bool Remove(const std::string&);
  bool Remove(TrackedObject*);

  void Clear();

  void Synch();

  Iterator<TrackedObject*> GetObjects() const;

private:
  std::map<std::string,TrackedObject*> objects_;
};

template<typename TrackedObjectType,
         typename = std::enable_if_t<std::is_base_of_v<TrackedObject,TrackedObjectType>>>
Iterator<TrackedObjectType*> GetObjects(const TrackedObjectDict& dict) {
  auto it = dict.GetObjects();
  auto it_impl = it.Reset();
  struct Filter {
    bool operator()(TrackedObject* o) const {
      return dynamic_cast<TrackedObjectType*>(o) != nullptr;
    }
  };
  struct Transform {
    typedef TrackedObjectType* out_value_type;
    typedef TrackedObject* in_value_type;
    TrackedObjectType* operator()(TrackedObject* val) const {
      return dynamic_cast<TrackedObjectType*>(val);
    }
  };
  auto it_filter_impl = new IteratorImpl_Filter<TrackedObject*,Filter,Transform>(it_impl);
  return Iterator<TrackedObjectType*>(it_filter_impl);
}
#endif // !NUMGEOM_FRAMEWORK_TRACKEDOBJECTDICT_H
