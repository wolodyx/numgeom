#ifndef NUMGEOM_APPQT_GIZMOWIDGET_H
#define NUMGEOM_APPQT_GIZMOWIDGET_H

#include "qgroupbox.h"

class Gizmo;
class GizmoObserver;

/** \class GizmoWidget
\brief Обобщенная UI-часть гизма, ui-виджет.
*/
class GizmoWidget : public QGroupBox {
  Q_OBJECT;
 public:
  GizmoWidget(Gizmo*, QWidget* = nullptr);
  ~GizmoWidget();
  Gizmo* GetGizmo();

  //! Метод вызывается, если содержимое гизмо изменилось.
  //! Производный класс переопределяет метод для обновления своего содержимого.
  virtual void UpdateContent() {}

  virtual void GizmoActivated() {}

  virtual void GizmoEnabled(bool enabled);

 protected:
  //! Метод активирует/деактивирует оповещение об изменении содержимого
  //! связанного гизмы (методов `UpdateContent`, `GizmoActivated`, `GizmoEnabled`).
  //! Метод позволяет исключить бесконечные циклические вызовы, возникающие при
  //! изменении полей ввода Ui-части.
  void EnableNotification(bool enable = true);

 private:
  Gizmo* gizmo_;
  GizmoObserver* observer_;
};
#endif // !NUMGEOM_APPQT_GIZMOWIDGET_H
