#ifndef NUMGEOM_FRAMEWORK_SCENEWIDGET_AXISINDICATOR_H
#define NUMGEOM_FRAMEWORK_SCENEWIDGET_AXISINDICATOR_H

#include "numgeom/framework_export.h"
#include "numgeom/scenewidget.h"

class SceneWidget_AxisIndicator : public SceneWidget {
 public:
  SceneWidget_AxisIndicator(Scene*);
  virtual ~SceneWidget_AxisIndicator();

 private:
  Drawable* sphere_;
  Drawable* x_cylinder_;
  Drawable* y_cylinder_;
  Drawable* z_cylinder_;
  Drawable* x_cone_;
  Drawable* y_cone_;
  Drawable* z_cone_;
};
#endif // !NUMGEOM_FRAMEWORK_SCENEWIDGET_AXISINDICATOR_H
