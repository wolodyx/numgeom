#ifndef NUMGEOM_FRAMEWORK_SCENEOBJECT_H
#define NUMGEOM_FRAMEWORK_SCENEOBJECT_H

#include <vector>

#include "numgeom/alignedboundbox.h"
#include "numgeom/framework_export.h"
#include "numgeom/iterator.h"
#include "numgeom/trackedobject.h"

class Drawable;
class Scene;
class SceneObjectImpl;

/**
\class SceneObject
\brief Объект сцены, составленный из списка `Drawable`-объектов.
*/
class FRAMEWORK_EXPORT SceneObject : public TrackedObject {
 public:
  SceneObject(Scene*);
  virtual ~SceneObject();

  bool IsVisible() const;
  void SetVisible(bool visible = true);

  bool IsPickable() const;
  void DisablePicking();
  void EnablePicking();

  AlignedBoundBox GetBoundBox() const;

  size_t DrawablesCount() const;

  Iterator<Drawable*> Drawables() const;

 protected:
  template<typename DrawableType, class... _Types>
  DrawableType* AddDrawable(_Types&&... _Args) {
    auto item = new DrawableType(this, _Args...);
    this->Insert(item);
    return item;
  }

  void Insert(Drawable*);
  bool Remove(Drawable*);

 private:
  SceneObject(const SceneObject&) = delete;
  SceneObject& operator=(const SceneObject&) = delete;

 private:
  SceneObjectImpl* impl_;
};
#endif // !NUMGEOM_FRAMEWORK_SCENEOBJECT_H
