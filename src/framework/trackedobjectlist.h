#ifndef NUMGEOM_FRAMEWORK_TRACKEDOBJECTLIST_H
#define NUMGEOM_FRAMEWORK_TRACKEDOBJECTLIST_H

#include <list>

#include "numgeom/iterator.h"
#include "numgeom/iteratorimpl.h"

class TrackedObject;

class TrackedObjectList {
 public:
  TrackedObjectList();

  ~TrackedObjectList();

  bool IsEmpty() const;

  //! Количество объектов в списке.
  size_t GetObjectsCount() const;

  Iterator<TrackedObject*> GetObjects() const;

  //! Итератор по всем объектом, включая исключенным из списка актуальных.
  Iterator<TrackedObject*> GetAllObjects() const;

  void Insert(TrackedObject*);

  bool Remove(TrackedObject*);

  void Clear();

  void Synch();

 private:
  std::list<TrackedObject*> objects_;
};

template<typename TrackedObjectType,
         typename = std::enable_if_t<std::is_base_of_v<TrackedObject,TrackedObjectType>>>
Iterator<TrackedObjectType*> GetObjects(const TrackedObjectList& list) {
  auto it = list.GetObjects();
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

#endif // !NUMGEOM_FRAMEWORK_TRACKEDOBJECTLIST_H
