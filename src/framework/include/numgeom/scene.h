#ifndef NUMGEOM_FRAMEWORK_SCENE_H
#define NUMGEOM_FRAMEWORK_SCENE_H

#include <list>

#include "numgeom/alignedboundbox.h"
#include "numgeom/framework_export.h"
#include "numgeom/iterator.h"

class SceneObject;

/**
\class Scene
\brief Сцена -- набор объектов для рендеринга.

Сцена состоит из так называемых 'объектов' типа `SceneObject`.
*/
class FRAMEWORK_EXPORT Scene {
 public:
  Scene();
  ~Scene();

  AlignedBoundBox GetBoundBox() const;

  Iterator<SceneObject*> Objects() const;

  void Clear();

  //! Добавление в сцену объекта по заданному типу и аргументам конструирования.
  template<typename ObjectType, class... _Types>
  SceneObject* AddObject(_Types&&... _Args) {
    auto item = new ObjectType(this, _Args...);
    objects_.push_back(item);
    has_changes_ = true;
    return item;
  }

  bool HasChanges() const;
  void ClearChanges();

 private:
  Scene(const Scene&) = delete;
  Scene& operator=(const Scene&) = delete;

 private:
  std::list<SceneObject*> objects_;
  bool has_changes_ = true;
};
#endif // !NUMGEOM_FRAMEWORK_SCENE_H
