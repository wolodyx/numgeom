#ifndef NUMGEOM_FRAMEWORK_SCENEWIDGET_ARROW_H
#define NUMGEOM_FRAMEWORK_SCENEWIDGET_ARROW_H

#include "numgeom/framework_export.h"
#include "numgeom/scenewidget.h"

class SceneWidget_Arrow : public SceneWidget {
 public:
  SceneWidget_Arrow(Scene*);
  virtual ~SceneWidget_Arrow();

 private:
  Drawable* head_;
  Drawable* shaft_;
  Drawable* base_;
};
#endif // !NUMGEOM_FRAMEWORK_SCENEWIDGET_ARROW_H
