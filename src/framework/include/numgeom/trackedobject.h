#ifndef NUMGEOM_FRAMEWORK_TRACKEDOBJECT_H
#define NUMGEOM_FRAMEWORK_TRACKEDOBJECT_H

class TrackedObjectList;

/** \class TrackedObject
\brief Класс объекта, для которого необходимо отслеживать состояние для
  последующей синхронизации со связанным объектом.
*/
class TrackedObject {
 public:
  enum class State {
    New,
    Clean,
    Dirty,
    Removed,
    Delete
  };

 public:
  TrackedObject();
  virtual ~TrackedObject();
  State GetState() const;
  bool IsDirty() const { return GetState() == State::Dirty; }
  bool IsRemoved() const { return GetState() == State::Removed; }
  bool IsDeleted() const { return GetState() == State::Delete; }
  void Sync();

 protected:
  void SetDirty();
  void AddSubObjects(const TrackedObjectList*);

 private:
  friend class TrackedObjectDict;
  friend class TrackedObjectList;
  void MarkForDeletion();

 private:
  State object_state_ = State::New;
  const TrackedObjectList* sub_objects_ = nullptr;
};
#endif // !NUMGEOM_FRAMEWORK_TRACKEDOBJECT_H
