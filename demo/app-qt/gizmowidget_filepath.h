#ifndef NUMGEOM_APPQT_GIZMOWIDGET_FILEPATH_H
#define NUMGEOM_APPQT_GIZMOWIDGET_FILEPATH_H

#include "gizmowidget.h"

class Gizmo_FilePath;

class GizmoWidget_FilePath : public GizmoWidget {
  Q_OBJECT;
 public:
  GizmoWidget_FilePath(Gizmo_FilePath*, QWidget* = nullptr);
  ~GizmoWidget_FilePath();
  void UpdateContent() override;

 private slots:
  void onOpenDialog();
  void onFilepathChanged(const QString&);

 private:
  class Ui;
  Ui* ui_;
};
#endif // !NUMGEOM_APPQT_GIZMOWIDGET_FILEPATH_H
