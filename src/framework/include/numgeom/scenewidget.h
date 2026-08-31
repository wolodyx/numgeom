#ifndef NUMGEOM_FRAMEWORK_SCENEWIDGET_H
#define NUMGEOM_FRAMEWORK_SCENEWIDGET_H

#include "numgeom/framework_enums.h"
#include "numgeom/framework_export.h"
#include "numgeom/sceneobject.h"

class FRAMEWORK_EXPORT SceneWidget : public SceneObject {
 public:
  SceneWidget(Scene*);
  virtual ~SceneWidget();

  //! Вызывается на событие перетаскивания объекта `Drawable` мышкой.
  virtual void Drag(Drawable*, const glm::vec3& dir) {}

  //! Вызывается на событие двойного клика мышкой на объект `Drawable`.
  virtual void DoubleClick(MouseButton, Drawable*) {}

  //! Вызывается на событие входа указателя мыши на объект `Drawable`.
  virtual void Enter(Drawable*) {}

  //! Вызывается на событие выхода указателя мыши с объекта `Drawable`.
  virtual void Exit(Drawable*) {}
};
#endif // !NUMGEOM_FRAMEWORK_SCENEWIDGET_H
