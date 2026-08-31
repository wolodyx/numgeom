#ifndef NUMGEOM_APPQT_GIZMOWIDGET_PLACEIN3D_H
#define NUMGEOM_APPQT_GIZMOWIDGET_PLACEIN3D_H

#include "gizmowidget.h"

class Gizmo_PlaceIn3d;

class GizmoWidget_PlaceIn3d : public GizmoWidget {
  Q_OBJECT;
 public:
  GizmoWidget_PlaceIn3d(Gizmo_PlaceIn3d*, QWidget* = nullptr);
  ~GizmoWidget_PlaceIn3d();
  void UpdateContent() override;

 private slots:
  void onCoordEdited(const QString&);
  void onCoordEditingFinished();

 private:
  class Ui;
  Ui* ui_;
};
#endif // !NUMGEOM_APPQT_GIZMOWIDGET_PLACEIN3D_H
