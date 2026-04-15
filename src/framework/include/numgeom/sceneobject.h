#ifndef NUMGEOM_FRAMEWORK_SCENEOBJECT_H
#define NUMGEOM_FRAMEWORK_SCENEOBJECT_H

#include <vector>

#include "numgeom/alignedboundbox.h"
#include "numgeom/framework_export.h"
#include "numgeom/iterator.h"

class Drawable;
class Scene;

/**
\class SceneObject
\brief Объект сцены, составленный из списка `Drawable`-объектов.
*/
class FRAMEWORK_EXPORT SceneObject {
 public:
  SceneObject(Scene*);
  virtual ~SceneObject();

  AlignedBoundBox GetBoundBox() const;

  size_t DrawablesCount() const;

  Iterator<Drawable*> Drawables() const;

  bool HasChanges() const;
  void ClearChanges();

 protected:
  template<typename DrawableType, class... _Types>
  DrawableType* AddDrawable(_Types&&... _Args) {
    auto item = new DrawableType(this, _Args...);
    drawables_.push_back(item);
    has_changes_ = true;
    return item;
  }

  bool Remove(Drawable*);

 private:
  Scene* scene_;
  std::vector<Drawable*> drawables_;
  bool has_changes_ = false;
};
#endif // !NUMGEOM_FRAMEWORK_SCENEOBJECT_H
