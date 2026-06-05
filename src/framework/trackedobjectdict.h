#ifndef NUMGEOM_FRAMEWORK_TRACKEDOBJECTDICT_H
#define NUMGEOM_FRAMEWORK_TRACKEDOBJECTDICT_H

#include <map>
#include <string>

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

private:
  std::map<std::string,TrackedObject*> objects_;
};
#endif // !NUMGEOM_FRAMEWORK_TRACKEDOBJECTDICT_H
