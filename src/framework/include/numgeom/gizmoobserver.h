#ifndef NUMGEOM_FRAMEWORK_GIZMOOBSERVER_H
#define NUMGEOM_FRAMEWORK_GIZMOOBSERVER_H

#include "numgeom/framework_export.h"

class Gizmo;

/** \class GizmoObserver
\brief Наблюдатель за состоянием виджета.
*/
class FRAMEWORK_EXPORT GizmoObserver
{
public:

  GizmoObserver();

  virtual ~GizmoObserver();

  //! Событие о том, что содержимое виджета изменилось.
  //! В ответ на это команда может, например, перерисовать фантом.
  virtual void ContentUpdated(Gizmo*) = 0;

  //! Событие о том, что виджет активизировался.
  virtual void Activated(Gizmo*) = 0;

  //! Событие о том, что виджет отключился/включился.
  virtual void Enabled(Gizmo*, bool enabled) = 0;

  //! Управление разрешением на оповещение.
  //! Если запретить оповещение (`observer->enable(false)`), то
  //! наблюдатель не получит извещения об изменении виджета.
  void Enable(bool enabled = true);

  bool Enabled() const;

private:
  bool enabled_ = true;
};
#endif // !NUMGEOM_FRAMEWORK_GIZMOOBSERVER_H
