#ifndef NUMGEOM_FRAMEWORK_SCENEWIDGET_H
#define NUMGEOM_FRAMEWORK_SCENEWIDGET_H

#include "numgeom/framework_enums.h"
#include "numgeom/framework_export.h"
#include "numgeom/sceneobject.h"

class FRAMEWORK_EXPORT SceneWidget : public SceneObject {
 public:
  SceneWidget(Scene*);
  virtual ~SceneWidget();
  virtual void Drag(Drawable*, const glm::vec3& dir) {};
  virtual void DoubleClick(MouseButton, Drawable*) {};
};
#endif // !NUMGEOM_FRAMEWORK_SCENEWIDGET_H
