#ifndef NUMGEOM_APPQT_GIZMOWIDGET_REALNUMBER_H
#define NUMGEOM_APPQT_GIZMOWIDGET_REALNUMBER_H

#include "gizmowidget.h"

class Gizmo_RealNumber;

class GizmoWidget_RealNumber : public GizmoWidget {
  Q_OBJECT;
 public:
  GizmoWidget_RealNumber(Gizmo_RealNumber*, QWidget* = nullptr);
  ~GizmoWidget_RealNumber();
  void UpdateContent() override;

 private:
  void SetInvalid(bool invalid = true);

 private slots:
  void onSliderValueChanged(int);
  void onValueChanged(const QString&);

 private:
  class Ui;
  Ui* ui_;
};
#endif // !NUMGEOM_APPQT_GIZMOWIDGET_REALNUMBER_H
